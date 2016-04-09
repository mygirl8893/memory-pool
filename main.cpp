#include "src/MicroCore.h"
#include "src/CmdLineOptions.h"
#include "src/tools.h"


#include "ext/member_checker.h"

#include "ext/minicsv.h"
#include "ext/format.h"

#include "ext/rapidjson/document.h"

#include <iostream>
#include <string>
#include <vector>

using boost::filesystem::path;
using request = cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request;
using response = cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response;
using http_simple_client = epee::net_utils::http::http_simple_client;

using namespace fmt;
using namespace std;


// without this it wont work. I'm not sure what it does.
// it has something to do with locking the blockchain and tx pool
// during certain operations to avoid deadlocks.

namespace epee {
    unsigned int g_test_dbg_lock_sleep = 0;
}



// defeine a checker to test if a structure has "tx_blob"
// member variable. I used modified daemon with few extra
// bits and peaces here and there. One of them is
// tx_blob in cryptonote::tx_info structure
DEFINE_MEMBER_CHECKER(tx_blob);


int main(int ac, const char* av[]) {

    // get command line options
    xmreg::CmdLineOptions opts {ac, av};

    auto help_opt = opts.get_option<bool>("help");

    // if help was chosen, display help text and finish
    if (*help_opt)
    {
        return 0;
    }

    bool HAVE_TX_BLOB {HAS_MEMBER(cryptonote::tx_info, tx_blob)};


    boost::mutex m_daemon_rpc_mutex;

    request req;
    response res;

    http_simple_client m_http_client;

    m_daemon_rpc_mutex.lock();

    bool r = epee::net_utils::invoke_http_json_remote_command2(
              string("http:://127.0.0.1:18081") + "/get_transaction_pool",
              req, res, m_http_client, 200000);

    m_daemon_rpc_mutex.unlock();

    if (!r)
    {
        cerr << "Error connectining to daeaomn" << endl;
        return 1;
    }


    cout << res.transactions.size() << endl;

    for (const cryptonote::tx_info& _tx_info: res.transactions)
    {

        rapidjson::Document json;

        if (json.Parse(_tx_info.tx_json.c_str()).HasParseError())
        {
            cerr << ("Failed to parse JSON") << endl;
            continue;
        }


        cout << "id: " << _tx_info.id_hash << std::endl
             << _tx_info.tx_json << std::endl
             << "blob_size: " << _tx_info.blob_size << std::endl
             << "fee: " << cryptonote::print_money(_tx_info.fee) << std::endl
             << "receive_time: " << _tx_info.receive_time << std::endl
             << "kept_by_block: " << (_tx_info.kept_by_block ? 'T' : 'F') << std::endl
             << "max_used_block_height: " << _tx_info.max_used_block_height << std::endl
             << "max_used_block_id: " << _tx_info.max_used_block_id_hash << std::endl
             << "last_failed_height: " << _tx_info.last_failed_height << std::endl
             << "last_failed_id: " << _tx_info.last_failed_id_hash << std::endl
             << /*"tx_blob:"  << _tx_info.tx_blob << */ endl;


        if (HAVE_TX_BLOB)
        {
            cryptonote::transaction tx;

            if (!cryptonote::parse_and_validate_tx_from_blob(
                    _tx_info.tx_blob, tx))
            {
                cerr << "Cant parse tx from blob" << endl;
            }

            cout << tx.vin.size() << endl;
        }

    }

    cout << "\nEnd of program." << endl;

    return 0;
}