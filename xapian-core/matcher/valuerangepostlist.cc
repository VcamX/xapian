/** @file valuerangepostlist.cc
 * @brief Return document ids matching a range test on a specified doc value.
 */
/* Copyright 2007 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <config.h>

#include "autoptr.h"
#include "omassert.h"
#include "document.h"
#include "utils.h"
#include "valuerangepostlist.h"

using namespace std;

Xapian::doccount
ValueRangePostList::get_termfreq_min() const
{
    return 0;
}

Xapian::doccount
ValueRangePostList::get_termfreq_est() const
{
    AssertParanoid(!db || db_size == db->get_doccount());
    // FIXME: It's hard to estimate well - perhaps consider the values of
    // begin and end?
    return db_size / 2;
}

Xapian::doccount
ValueRangePostList::get_termfreq_max() const
{
    AssertParanoid(!db || db_size == db->get_doccount());
    return db_size;
}

Xapian::weight
ValueRangePostList::get_maxweight() const
{
    return 0;
}

Xapian::docid
ValueRangePostList::get_docid() const
{
    Assert(current);
    Assert(db);
    return current;
}

Xapian::weight
ValueRangePostList::get_weight() const
{
    Assert(db);
    return 0;
}

Xapian::doclength
ValueRangePostList::get_doclength() const
{
    Assert(db);
    return 0;
}

Xapian::weight
ValueRangePostList::recalc_maxweight()
{
    Assert(db);
    return 0;
}

PositionList *
ValueRangePostList::read_position_list()
{
    Assert(db);
    return NULL;
}

PositionList *
ValueRangePostList::open_position_list() const
{
    Assert(db);
    return NULL;
}

PostList *
ValueRangePostList::next(Xapian::weight)
{
    Assert(db);
    AssertParanoid(lastdocid == db->get_lastdocid());
    while (current < lastdocid && ++current != 0) {
	// Wrap both calls to open_document in a try-catch block; in theory,
	// only the call with lazy=false should raise the
	// Xapian::DocNotFoundError, but the inmemory database ignores the lazy
	// flag, so we need to catch the exception from the lazy=true call too.
	try {
	    AutoPtr<Xapian::Document::Internal> doc(db->open_document(current, true));
	    string v = doc->get_value(valno);
	    if (v >= begin && v <= end) {
		if (v.empty()) {
		    delete db->open_document(current, false);
		}
		return NULL;
	    }
	} catch (const Xapian::DocNotFoundError &) {
	    continue;
	}
    }
    db = NULL;
    return NULL;
}

PostList *
ValueRangePostList::skip_to(Xapian::docid did, Xapian::weight w_min)
{
    Assert(db);
    if (did <= current) return NULL;
    current = did - 1;
    return ValueRangePostList::next(w_min);
}

PostList *
ValueRangePostList::check(Xapian::docid did, Xapian::weight, bool &valid)
{
    Assert(db);
    if (did <= current) {
	valid = true;
	return NULL;
    }
    AssertParanoid(lastdocid == db->get_lastdocid());
    if (did > lastdocid) {
	db = NULL;
	valid = true;
	return NULL;
    }
    try {
	current = did;
	AutoPtr<Xapian::Document::Internal> doc(db->open_document(current, true));
	string v = doc->get_value(valno);
	if (v >= begin && v <= end) {
	    valid = true;
	    return NULL;
	}
    } catch (const Xapian::DocNotFoundError &) {
	// That document doesn't exist.
	// FIXME: this could throw and catch a lot of exceptions!
    }
    valid = false;
    return NULL;
}

bool
ValueRangePostList::at_end() const
{
    return (db == NULL);
}

string
ValueRangePostList::get_description() const
{
    string desc = "ValueRangePostList(";
    desc += om_tostring(valno);
    desc += ", ";
    desc += begin;
    desc += ", ";
    desc += end;
    desc += ")";
    return desc;
}
