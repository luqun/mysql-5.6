/*
   Copyright (c) 2023 Meta, Inc

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/* This C++ file's header file */
#include "rdb_native_dd.h"
#include <cstring>

/* MySQL header files */
#include "sql/dd/types/table.h"  // dd::Table

/* MyRocks header files */
#include "ha_rocksdb.h"
#include "sql/plugin_table.h"
#include "storage/rocksdb/ha_rocksdb_proto.h"
#include "storage/rocksdb/rdb_datadic.h"

namespace myrocks {
std::unordered_set<dd::Object_id> native_dd::s_dd_table_ids = {};

bool native_dd::is_dd_table_id(dd::Object_id id) {
  return (native_dd::s_dd_table_ids.find(id) !=
          native_dd::s_dd_table_ids.end());
}

int native_dd::reject_if_dd_table(const dd::Table *) {
  //  if (table_def != nullptr && is_dd_table_id(table_def->se_private_id())) {
  //    my_error(ER_NOT_ALLOWED_COMMAND, MYF(0));
  //    return HA_ERR_UNSUPPORTED;
  //  }

  return (0);
}

void native_dd::insert_dd_table_ids(dd::Object_id dd_table_id) {
  s_dd_table_ids.insert(dd_table_id);
}

void native_dd::clear_dd_table_ids() { s_dd_table_ids.clear(); }

void rocksdb_dict_register_dd_table_id(dd::Object_id dd_table_id) {
  native_dd::insert_dd_table_ids(dd_table_id);
};

bool rocksdb_dict_get_server_version(uint *version) {
  return rdb_get_dict_manager()
      ->get_dict_manager_selector_non_const(false /*is_tmp_table*/)
      ->get_server_version(version);
};

bool rocksdb_dict_set_server_version() {
  return rdb_get_dict_manager()
      ->get_dict_manager_selector_non_const(false /*is_tmp_table*/)
      ->set_server_version();
};

bool rocksdb_is_supported_system_table([[maybe_unused]] const char *db_name,
                                       [[maybe_unused]] const char *tbl_name,
                                       bool is_sql_layer_system_table) {
  DBUG_EXECUTE_IF("ddse_rocksdb", {
    if (strcmp(db_name, "mysql") == 0 &&
        strcmp(tbl_name, "password_history") == 0) {
      return true;
    }
  });
  return is_sql_layer_system_table;
}

/** Initialize RocksDB for being used to store the DD tables.
Create the required files according to the dict_init_mode.
Create strings representing the required DDSE tables, i.e.,
tables that RocksDB expects to exist in the DD,
and add them to the appropriate out parameter.

@param[in]	dict_init_mode	How to initialize files

@param[in]	version		Target DD version if a new server
                                is being installed.
                                0 if restarting an existing server.

@param[out]	tables		List of SQL DDL statements
                                for creating DD tables that
                                are needed by the DDSE.

@param[out]	tablespaces	List of meta data for predefined
                                tablespaces created by the DDSE.

@retval	true			An error occurred.
@retval	false			Success - no errors. */
bool rocksdb_ddse_dict_init(dict_init_mode_t dict_init_mode [[maybe_unused]],
                            uint version [[maybe_unused]],
                            List<const dd::Object_table> *tables
                            [[maybe_unused]],
                            List<const Plugin_tablespace> *tablespaces) {
  DBUG_TRACE;

  static Plugin_tablespace dd_space("mysql", "", "", "", rocksdb_hton_name);
  tablespaces->push_back(&dd_space);
  return false;
}

/** Perform high-level recovery in InnoDB as part of initializing the
data dictionary.
@param[in]	dict_recovery_mode	How to do recovery
@param[in]	version			Target DD version if a new
                                        server is being installed.
                                        Actual DD version if restarting
                                        an existing server.
@retval	true				An error occurred.
@retval	false				Success - no errors. */
bool rocksdb_dict_recover(dict_recovery_mode_t dict_recovery_mode, uint) {
  switch (dict_recovery_mode) {
    case DICT_RECOVERY_INITIALIZE_SERVER:
      return (false);
    default:
      return false;
  }
  return false;
}

bool rocksdb_upgrade_space_version(dd::Tablespace *) { return false; }

int rocksdb_dd_upgrade_finish(THD *, bool) { return false; }

/** Invalidate an entry or entries for partitoined table from the dict cache.
@param[in]	schema_name	Schema name
@param[in]	table_name	Table name */
void rocksdb_dict_cache_reset(const char *, const char *) {}

/** Invalidate user table dict cache after Replication Plugin recovers. Table
definition could be different with XA commit/rollback of DDL operations */
void rocksdb_dict_cache_reset_tables_and_tablespaces() {}

/** Check if RocksDB is in a mode where the data dictionary is read-only.
 */
bool rocksdb_is_dict_readonly() {
  DBUG_TRACE;
  return false;
}

}  // namespace myrocks
