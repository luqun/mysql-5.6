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
#pragma once

/* C++ standard header files */
#include <sys/types.h>
#include <unordered_set>

/* MySQL header files */
#include "sql/dd/object_id.h"
#include "sql/handler.h"
#include "sql/plugin_table.h"

namespace dd {
class Table;
}

namespace myrocks {

void rocksdb_dict_register_dd_table_id(dd::Object_id dd_table_id);
bool rocksdb_dict_get_server_version(uint *version);
bool rocksdb_dict_set_server_version();
bool rocksdb_is_supported_system_table(const char *, const char *, bool);
bool rocksdb_is_dict_readonly();

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
bool rocksdb_ddse_dict_init(dict_init_mode_t dict_init_mode, uint version,
                            List<const dd::Object_table> *tables,
                            List<const Plugin_tablespace> *tablespaces);

bool rocksdb_dict_recover(dict_recovery_mode_t dict_recovery_mode,
                          uint version);
bool rocksdb_upgrade_space_version(dd::Tablespace *);

int rocksdb_dd_upgrade_finish(THD *thd, bool failed_upgrade);

/** Invalidate an entry or entries for partitoined table from the dict cache.
@param[in]	schema_name	Schema name
@param[in]	table_name	Table name */
void rocksdb_dict_cache_reset(const char *schema_name, const char *table_name);
/** Invalidate user table dict cache after Replication Plugin recovers. Table
definition could be different with XA commit/rollback of DDL operations */
void rocksdb_dict_cache_reset_tables_and_tablespaces();

class native_dd {
 private:
  /* Set of ids of DD tables */
  static std::unordered_set<dd::Object_id> s_dd_table_ids;

 public:
  static bool is_dd_table_id(dd::Object_id id);

  static void insert_dd_table_ids(dd::Object_id dd_table_id);

  static void clear_dd_table_ids();

  static int reject_if_dd_table(const dd::Table *table_def);
};

}  // namespace myrocks
