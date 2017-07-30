/**
 * Copyright (C) 2017 Cisco Inc.
 *
 * This program is free software: you can redistribute it and/or  modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// @author Changxue Deng <chadeng@cisco.com>

#ifndef __MABAIN_H__
#define __MABAIN_H__

namespace mabain {

class CONSTS
{
public:
    static const int ACCESS_MODE_READER;
    static const int ACCESS_MODE_WRITER;
    static const int OPTION_ALL_PREFIX;
    static const int OPTION_FIND_AND_DELETE;
    static const int MAX_KEY_LENGHTH;
    static const int MAX_DATA_SIZE;
};

}

#endif