/* database.h: database class declarations
 *
 * ----START-LICENCE----
 * Copyright 1999,2000,2001 BrightStation PLC
 * Copyright 2002 Ananova Ltd
 * Copyright 2002,2003,2004 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 * -----END-LICENCE-----
 */

#ifndef OM_HGUARD_DATABASE_H
#define OM_HGUARD_DATABASE_H

#include <string>

#include <xapian/types.h>
#include <xapian/document.h>
#include "omdebug.h"
#include "emptypostlist.h"

using namespace std;

class LeafPostList;
class LeafTermList;
class NetworkDatabase;

namespace Xapian {
class TermIterator::Internal;
class PositionIterator::Internal;
}

typedef Xapian::TermIterator::Internal TermList;
typedef Xapian::PositionIterator::Internal PositionList;

namespace Xapian {

/** Base class for databases.
 */
class Database::Internal : public Xapian::Internal::RefCntBase {
    private:
	/// Copies are not allowed.
	Internal(const Internal &);

	/// Assignment is not allowed.
	void operator=(const Internal &);

	/// Flag recording whether a transaction is in progress
	bool transaction_in_progress;

    protected:
    	/** Create a database - called only by derived classes.
	 */
	Internal();

	/** Internal method to perform cleanup when a writable database is
	 *  destroyed with unflushed changes.
	 *
	 *  A derived class' destructor should call this method before
	 *  destroying the database to ensure that no sessions or
	 *  transactions are in progress at destruction time.
	 *
	 *  Note that it is not safe to throw exceptions from destructors,
	 *  so this method will catch and discard any exceptions.
	 */
	void dtor_called();

	/** Method definied by subclass to actually open a posting list.
	 *
	 *  This is a list of all the documents which contain a given term.
	 *
	 *  @param tname  The term whose posting list is being requested.
	 *
	 *  @return       A pointer to the newly created posting list.
	 *                This object must be deleted by the caller after
	 *                use.
	 */
	virtual LeafPostList * do_open_post_list(const string & tname) const = 0;

    public:
	/** Destroy the database.
	 *  
	 *  This method should not be called until all objects using the
	 *  database have been cleaned up.
	 *
	 *  If any transactions are in progress, they should
	 *  be finished by cancel_transaction() or
	 *  commit_transaction() - if this is not done, the destructor
	 *  will attempt to clean things up by cancelling the transaction,
	 *  but any errors produced by these operations will not be reported.
	 */
        virtual ~Internal();

	/** Send a keep-alive signal to a remote database, to stop
	 *  it from timing out.
	 */
	virtual void keep_alive() const;

	//////////////////////////////////////////////////////////////////
	// Database statistics:
	// ====================

	/** Return the number of docs in this (sub) database.
	 */
	virtual Xapian::doccount get_doccount() const = 0;

	/** Return the average length of a document in this (sub) database.
	 *
	 *  See Database::Internal::get_doclength() for the meaning of document
	 *  length within Xapian.
	 */
	virtual Xapian::doclength get_avlength() const = 0;

	/** Get the length of a given document.
	 *
	 *  Document length, for the purposes of Xapian, is defined to be
	 *  the number of instances of terms within a document.  Expressed
	 *  differently, the sum of the within document frequencies over
	 *  all the terms in the document.
	 *
	 *  @param did  The document id of the document whose length is
	 *              being requested.
	 */
	virtual Xapian::doclength get_doclength(Xapian::docid did) const = 0;

	/** Return the number of documents indexed by a given term.  This
	 *  may be an approximation, but must be an upper bound (ie,
	 *  greater or equal to the true value), and should be as accurate
	 *  as possible.
	 *
	 *  @param tname  The term whose term frequency is being requested.
	 */
	virtual Xapian::doccount get_termfreq(const string & tname) const = 0;

	/** Return the total number of occurrences of the given term.  This
	 *  is the sum of the number of ocurrences of the term in each
	 *  document: ie, the sum of the within document frequencies of the
	 *  term.
	 *
	 *  @param tname  The term whose collection frequency is being
	 *                requested.
	 */
	virtual Xapian::termcount get_collection_freq(const string & tname) const = 0;

	/** Check whether a given term is in the database.
	 *
	 *  This method should normally be functionally equivalent to
	 *  (get_termfreq() != 0), but this equivalence should not be
	 *  relied upon.  For example, in a database which allowed
	 *  deleting, get_termfreq() might return the term frequency before
	 *  the term was deleted, whereas term_exists() might be more
	 *  up-to-date.
	 *
	 *  This method will also often be considerably more efficient than
	 *  get_termfreq().
	 *
	 *  @param tname  The term whose presence is being checked.
	 */
	virtual bool term_exists(const string & tname) const = 0;

	//////////////////////////////////////////////////////////////////
	// Data item access methods:
	// =========================

	/** Open a posting list.
	 *
	 *  This is a list of all the documents which contain a given term.
	 *
	 *  @param tname  The term whose posting list is being requested.
	 *
	 *  @return       A pointer to the newly created posting list.
	 *                This object must be deleted by the caller after
	 *                use.
	 */
	LeafPostList * open_post_list(const string & tname) const {
	    if (!term_exists(tname)) {
		DEBUGLINE(MATCH, tname + " is not in database.");
		// Term doesn't exist in this database.  However, we create
		// a (empty) postlist for it to help make distributed searching
		// cleaner (term might exist in other databases).
		return new EmptyPostList();
	    }
	    return do_open_post_list(tname);
	}

	/** Open a term list.
	 *
	 *  This is a list of all the terms contained by a given document.
	 *
	 *  @param did    The document id whose term list is being requested.
	 *
	 *  @return       A pointer to the newly created term list.
	 *                This object must be deleted by the caller after
	 *                use.
	 */
	virtual LeafTermList * open_term_list(Xapian::docid did) const = 0;

	/** Open an allterms list.
	 *
	 *  This is a list of all the terms in the database
	 *
	 *  @return       A pointer to the newly created allterms list.
	 *                This object must be deleted by the caller after
	 *                use.
	 */
	virtual TermList * open_allterms() const = 0;

	/** Open a position list for the given term in the given document.
	 *
	 *  @param did    The document id for which a position list is being
	 *                requested.
	 *  @param tname  The term for which a position list is being
	 *                requested.
	 *
	 *  @return       A pointer to the newly created position list.
	 *                This object must be deleted by the caller after
	 *                use.
	 */
	virtual PositionList * open_position_list(Xapian::docid did,
					const string & tname) const = 0;

	/** Open a document.
	 *
	 *  This is used to access the values and data associated with a
	 *  document.  See class Xapian::Document::Internal for further details.
	 *
	 *  @param did    The document id which is being requested.
	 *
	 *  @param lazy   Don't check the document exists immediately -
	 *                use from within the matcher where we know the
	 *                document exists, and don't want to read the
	 *                record when we just want the values.
	 *
	 *  @return       A pointer to the newly created document object.
	 *                This object must be deleted by the caller after
	 *                use.
	 */
	virtual Xapian::Document::Internal *
	open_document(Xapian::docid did, bool lazy = false) const = 0;

	/** Reopen the database to the latest available revision.
	 *
	 *  Some database implementations may do nothing.
	 */
	virtual void reopen() {
	    /* Default is to do nothing. */
	}

	//////////////////////////////////////////////////////////////////
	// Modifying the database:
	// =======================

	/** Flush modifications to the database.
	 *
	 *  See WritableDatabase::flush() for more information.
	 */
	virtual void flush() {
	    // Writable databases should override this method. 
	    Assert(false);
	}

 	/** Begin a transaction.
 	 *
 	 *  See WritableDatabase::begin_transaction() for more information.
 	 */
 	void begin_transaction();
 
 	/** Commit a transaction.
 	 *
 	 *  See WritableDatabase::commit_transaction() for more information.
 	 */
 	void commit_transaction();
 
 	/** Cancel a transaction.
 	 *
 	 *  See WritableDatabase::cancel_transaction() for more information.
 	 */
 	void cancel_transaction();

	/** Add a new document to the database.
	 *
	 *  See WritableDatabase::add_document() for more information.
	 */
	virtual Xapian::docid add_document(const Xapian::Document & /*document*/) {
	    // Writable databases should override this method. 
	    Assert(false);
	    return 0;
	}

	/** Delete a document in the database.
	 *
	 *  See WritableDatabase::delete_document() for more information.
	 */
	virtual void delete_document(Xapian::docid /*did*/) {
	    // Writable databases should override this method. 
	    Assert(false);
	}

	/** Replace a given document in the database.
	 *
	 *  See WritableDatabase::replace_document() for more information.
	 */
	virtual void replace_document(Xapian::docid /*did*/, const Xapian::Document & /*document*/) {
	    // Writable databases should override this method. 
	    Assert(false);
	}

	/** Request and later collect a document from the database.
	 *  Multiple documents can be requested with request_document(),
	 *  and then collected with collect_document().  Allows the backend
	 *  to optimise (e.g. the remote backend can start requests for all
	 *  the documents so they fetch in parallel).
	 *
	 *  If a backend doesn't support this, request_document() can be a
	 *  no-op and collect_document() the same as open_document().
	 */
	//@{
	virtual void request_document(Xapian::docid /*did*/) const { }

	virtual Xapian::Document::Internal * collect_document(Xapian::docid did) const {
	    return open_document(did);
	}
	//@}

	//////////////////////////////////////////////////////////////////
	// Introspection methods:
	// ======================

	/** Return a pointer to this object as a NetworkDatabase, or NULL.
	 *
	 *  This method is used by MultiMatch to decide whether to use a
	 *  LocalSubMatch or a RemoteSubMatch to perform a search over the
	 *  database.
	 */
	virtual const NetworkDatabase * as_networkdatabase() const {
	    return NULL;
	}
};

}

#endif /* OM_HGUARD_DATABASE_H */
