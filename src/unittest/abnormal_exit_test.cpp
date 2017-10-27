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

#include <iostream>

#include <gtest/gtest.h>

#include "../dict.h"
#include "../dict_mem.h"
#include "./test_key.h"

using namespace mabain;

namespace {

class AbnormalExitTest : public ::testing::Test
{
public:
    AbnormalExitTest() {
        remove_index = -1;
        key_type = MABAIN_TEST_KEY_TYPE_INT;
    }
    virtual ~AbnormalExitTest() {
    }
    virtual void SetUp() {
        std::string db_dir = "/var/tmp/mabain_test";
        unlink((db_dir + "/_mabain_h").c_str());
        unlink((db_dir + "/_dbfl").c_str());
        unlink((db_dir + "/_ibfl").c_str());
        db = new DB(db_dir, CONSTS::ACCESS_MODE_WRITER);
        if(!db->is_open()) {
            std::cerr << "failed to open db: " << db_dir << " " << db->StatusStr() << "\n";
            exit(0);
        }
    }
    virtual void TearDown() {
        if(db != NULL) {
            db->Close();
            delete db;
            db = NULL;
        }
    }

    void Populate(int count) {
        db->RemoveAll();
        std::string key_str;
        TestKey tkey(key_type);
        for(int key = 1; key <= count; key++) {
            key_str = tkey.get_key(key);
            db->Add(key_str, key_str);
        }
    }

    void SimulateAbnormalExit(int exception_type) {
        Dict *dict = db->GetDictPtr();
        IndexHeader *header = dict->GetHeader();
        DictMem *dmm = dict->GetMM();
        std::string key_str;
        TestKey tkey(key_type);
        int rval;

        switch(exception_type) {
            case EXCEP_STATUS_ADD_DATA_OFF:
                key_str = tkey.get_key(1278);
                db->Add(key_str, key_str + "_UPDATED", true);
                break;
            case EXCEP_STATUS_ADD_NODE:
                key_str = "***abc1";
                db->Add(key_str, key_str);
                key_str = "***abd1";
                db->Add(key_str, key_str);
                key_str = "***abe1";
                db->Add(key_str, key_str);
                key_str = "***ab";
                db->Add(key_str, key_str);
                break;
            case EXCEP_STATUS_CLEAR_EDGE:
                key_str = tkey.get_key(remove_index);
                rval = db->Remove(key_str);
                assert(rval == MBError::SUCCESS);
                break;
            case EXCEP_STATUS_REMOVE_EDGE:
                break;
        }

        header->excep_updating_status = exception_type;

        // Writer random data to simulate the db inconsistency
        srand(time(NULL));
        uint8_t buffer[4*4];
        int *ptr = (int *) buffer;
        for(int i = 0; i < 4; i++) {
            ptr[i] = (int) rand();
        }
        
        switch(exception_type) {
            case EXCEP_STATUS_ADD_EDGE:
                dmm->WriteData(buffer, EDGE_SIZE, header->excep_lf_offset);
                break;
            case EXCEP_STATUS_ADD_DATA_OFF:
                dmm->WriteData(buffer, OFFSET_SIZE, header->excep_lf_offset+EDGE_NODE_LEADING_POS);
                break;
            case EXCEP_STATUS_ADD_NODE:
                dmm->WriteData(buffer, NODE_EDGE_KEY_FIRST, header->excep_offset);
                break;
            case EXCEP_STATUS_REMOVE_EDGE:
                // Currently we cannot simulate this exception.
                //dmm->WriteData(buffer, OFFSET_SIZE, header->excep_lf_offset+EDGE_NODE_LEADING_POS);
                break;
            case EXCEP_STATUS_CLEAR_EDGE:
		dmm->WriteData(buffer, EDGE_SIZE, header->excep_lf_offset);
		break;
            default:
                break;
        }
    }

    int RecoverDB() {
        Dict *dict = db->GetDictPtr();
        int rval = dict->ExceptionRecovery();
        return rval;
    }

    int CheckDBConcistency(int count) {
        DB db_r("/var/tmp/mabain_test", CONSTS::ACCESS_MODE_READER);
        assert(db_r.is_open());
        std::string key_str;
        MBData mbd;
        TestKey tkey(key_type);
        int rval;
        int failed_cnt = 0;
        for(int key = 1; key <= count; key++) {
            key_str = tkey.get_key(key);
            rval = db_r.Find(key_str, mbd);

            if(key == remove_index) continue;
            if(rval != MBError::SUCCESS) {
                failed_cnt++;
                continue;
            }
            if(key_str != std::string((char*)mbd.buff, mbd.data_len)) {
                // Value may be updated
                if(key_str+"_UPDATED" != std::string((char*)mbd.buff, mbd.data_len)) {
                    failed_cnt++;
                }
            }
        }

        db_r.Close();
        return failed_cnt;
    }

    int CheckHalfDBConsistency(int count, bool check_even) {
        DB db_r("/var/tmp/mabain_test", CONSTS::ACCESS_MODE_READER);
        assert(db_r.is_open());
        std::string key_str;
        MBData mbd;
        TestKey tkey(key_type);
        int rval;
        int failed_cnt = 0;
        for(int key = 1; key <= count; key++) {
            key_str = tkey.get_key(key);
            rval = db_r.Find(key_str, mbd);

            if(check_even) {
                if(key % 2 == 1) continue;
            } else {
                if(key % 2 == 0) continue;
            }
            if(rval != MBError::SUCCESS) {
                failed_cnt++;
                continue;
            }
            if(key_str != std::string((char*)mbd.buff, mbd.data_len)) {
                // Value may be updated
                if(key_str+"_UPDATED" != std::string((char*)mbd.buff, mbd.data_len)) {
                    failed_cnt++;
                }
            }
        }

        db_r.Close();
        return failed_cnt;
    }

protected:
    DB *db;
    int remove_index;
    int key_type;
};

TEST_F(AbnormalExitTest, KEY_TYPE_INT_test)
{
    int count = 32331;
    int failed_cnt;
    int rval;

    key_type = MABAIN_TEST_KEY_TYPE_INT;

    Populate(count);
    SimulateAbnormalExit(EXCEP_STATUS_ADD_EDGE);
    failed_cnt = CheckDBConcistency(count);
    std::cout << "failed count before recovery: " << failed_cnt << "\n";

    rval = RecoverDB();
    EXPECT_EQ(rval, MBError::SUCCESS);

    failed_cnt = CheckDBConcistency(count);
    EXPECT_EQ(failed_cnt, 0);
}

TEST_F(AbnormalExitTest, KEY_TYPE_SHA1_test)
{
    int count = 18293;
    int failed_cnt;
    int rval;

    key_type = MABAIN_TEST_KEY_TYPE_SHA_128;

    Populate(count);
    SimulateAbnormalExit(EXCEP_STATUS_ADD_EDGE);
    failed_cnt = CheckDBConcistency(count);
    std::cout << "failed count before recovery: " << failed_cnt << "\n";

    rval = RecoverDB();
    EXPECT_EQ(rval, MBError::SUCCESS);

    failed_cnt = CheckDBConcistency(count);
    EXPECT_EQ(failed_cnt, 0);
}

TEST_F(AbnormalExitTest, KEY_TYPE_SHA256_test)
{
    int count = 5293;
    int failed_cnt;
    int rval;

    key_type = MABAIN_TEST_KEY_TYPE_SHA_256;

    Populate(count);
    SimulateAbnormalExit(EXCEP_STATUS_ADD_EDGE);
    failed_cnt = CheckDBConcistency(count);
    std::cout << "failed count before recovery: " << failed_cnt << "\n";

    rval = RecoverDB();
    EXPECT_EQ(rval, 0);

    failed_cnt = CheckDBConcistency(count);
    EXPECT_EQ(failed_cnt, 0);
}

TEST_F(AbnormalExitTest, KEY_TYPE_SHA1_ADD_DATA_OFF_test)
{
    int count = 18293;
    int failed_cnt;
    int rval;

    key_type = MABAIN_TEST_KEY_TYPE_SHA_128;

    Populate(count);
    SimulateAbnormalExit(EXCEP_STATUS_ADD_DATA_OFF);
    failed_cnt = CheckDBConcistency(count);
    std::cout << "failed count before recovery: " << failed_cnt << "\n";

    rval = RecoverDB();
    EXPECT_EQ(rval, MBError::SUCCESS);

    failed_cnt = CheckDBConcistency(count);
    EXPECT_EQ(failed_cnt, 0);
}

TEST_F(AbnormalExitTest, KEY_TYPE_INT_ADD_NODE_test)
{
    int count = 1829;
    int failed_cnt;
    int rval;

    key_type = MABAIN_TEST_KEY_TYPE_INT;

    Populate(count);
    SimulateAbnormalExit(EXCEP_STATUS_ADD_NODE);
    failed_cnt = CheckDBConcistency(count);
    std::cout << "failed count before recovery: " << failed_cnt << "\n";

    rval = RecoverDB();
    EXPECT_EQ(rval, MBError::SUCCESS);

    failed_cnt = CheckDBConcistency(count);
    EXPECT_EQ(failed_cnt, 0);
}

TEST_F(AbnormalExitTest, KEY_TYPE_INT_REMOVE_test)
{
    int count = 23234;
    int failed_cnt;
    int rval;

    key_type = MABAIN_TEST_KEY_TYPE_INT;
    remove_index = 2345;

    Populate(count);
    SimulateAbnormalExit(EXCEP_STATUS_CLEAR_EDGE);
    failed_cnt = CheckDBConcistency(count);
    std::cout << "failed count before recovery: " << failed_cnt << "\n";

    rval = RecoverDB();
    EXPECT_EQ(rval, 0);

    failed_cnt = CheckDBConcistency(count);
    EXPECT_EQ(failed_cnt, 0);
}

TEST_F(AbnormalExitTest, KEY_TYPE_INT_REMOVE_test_1)
{
    int count = 23234;
    int failed_cnt;
    int rval;

    key_type = MABAIN_TEST_KEY_TYPE_INT;
    remove_index = 1;

    Populate(count);
    SimulateAbnormalExit(EXCEP_STATUS_CLEAR_EDGE);
    failed_cnt = CheckDBConcistency(count);
    std::cout << "failed count before recovery: " << failed_cnt << "\n";

    rval = RecoverDB();
    EXPECT_EQ(rval, MBError::SUCCESS);

    failed_cnt = CheckDBConcistency(count);
    EXPECT_EQ(failed_cnt, 0);
}

TEST_F(AbnormalExitTest, KEY_TYPE_SHA_128_REMOVE_test)
{
    int count = 3934;
    int failed_cnt;
    int rval;

    key_type = MABAIN_TEST_KEY_TYPE_INT;
    remove_index = 1021;

    Populate(count);
    SimulateAbnormalExit(EXCEP_STATUS_CLEAR_EDGE);
    failed_cnt = CheckDBConcistency(count);
    std::cout << "failed count before recovery: " << failed_cnt << "\n";

    rval = RecoverDB();
    EXPECT_EQ(rval, MBError::SUCCESS);

    failed_cnt = CheckDBConcistency(count);
    EXPECT_EQ(failed_cnt, 0);
}

TEST_F(AbnormalExitTest, KEY_TYPE_INT_REMOVE_ODD_test)
{
    int count = 13234;
    int failed_cnt;
    int rval;

    key_type = MABAIN_TEST_KEY_TYPE_INT;

    Populate(count);
    for(int k = 1; k <= count; k++) {
        if(k % 2 == 0) continue;

        remove_index = k;
        SimulateAbnormalExit(EXCEP_STATUS_CLEAR_EDGE);
        rval = RecoverDB();
        EXPECT_EQ(rval, MBError::SUCCESS);
    } 

    failed_cnt = CheckHalfDBConsistency(count, true);
    EXPECT_EQ(failed_cnt, 0);
}

TEST_F(AbnormalExitTest, KEY_TYPE_SHA_256_REMOVE_EVEN_test)
{
    int count = 13234;
    int failed_cnt;
    int rval;

    key_type = MABAIN_TEST_KEY_TYPE_SHA_256;

    Populate(count); 
    for(int k = 1; k <= count; k++) {
        if(k % 2 == 1) continue;

        remove_index = k;
        SimulateAbnormalExit(EXCEP_STATUS_CLEAR_EDGE);
        rval = RecoverDB();
        EXPECT_EQ(rval, MBError::SUCCESS);
    }

    failed_cnt = CheckHalfDBConsistency(count, false);
    EXPECT_EQ(failed_cnt, 0);
}

}
