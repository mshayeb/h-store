/* This file is part of VoltDB.
 * Copyright (C) 2008-2010 VoltDB Inc.
 *
 * This file contains original code and/or modifications of original code.
 * Any modifications made by VoltDB Inc. are licensed under the following
 * terms and conditions:
 *
 * VoltDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * VoltDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with VoltDB.  If not, see <http://www.gnu.org/licenses/>.
 */
/* Copyright (C) 2008 by H-Store Project
 * Brown University
 * Massachusetts Institute of Technology
 * Yale University
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef VOLTDBENGINE_H
#define VOLTDBENGINE_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include <stdint.h>
#include "boost/shared_ptr.hpp"
#include "json_spirit/json_spirit.h"
#include "catalog/database.h"
#include "common/ids.h"
#include "common/Pool.hpp"
#include "common/MMAPMemoryManager.h"
#include "common/serializeio.h"
#include "common/types.h"
#include "common/valuevector.h"
#include "common/UndoLog.h"
#include "common/DummyUndoQuantum.hpp"
#include "common/SerializableEEException.h"
#include "common/Topend.h"
#include "common/DefaultTupleSerializer.h"
#include "logging/LogManager.h"
#include "logging/LogProxy.h"
#include "logging/StdoutLogProxy.h"
#include "stats/StatsAgent.h"
#include "storage/persistenttable.h"
#include "storage/mmap_persistenttable.h"
#include "common/ThreadLocalPool.h"

#ifdef ANTICACHE
#include "anticache/EvictedTupleAccessException.h"
#endif

// shorthand for ExecutionEngine versions generated by javah
#define ENGINE_ERRORCODE_SUCCESS 0
#define ENGINE_ERRORCODE_ERROR 1
#define ENGINE_ERRORCODE_NO_DATA 102

#define MAX_BATCH_COUNT 1000
#define MAX_PARAM_COUNT 1000 // or whatever

namespace boost {
template <typename T> class shared_ptr;
}

namespace catalog {
class Catalog;
class PlanFragment;
class Table;
class Statement;
class Cluster;
}

class VoltDBIPC;

namespace voltdb {

class AbstractExecutor;
class AbstractPlanNode;
class SerializeInput;
class SerializeOutput;
class Table;
class CatalogDelegate;
class ReferenceSerializeInput;
class ReferenceSerializeOutput;
class PlanNodeFragment;
class ExecutorContext;
class RecoveryProtoMsg;

/**
 * Represents an Execution Engine which holds catalog objects (i.e. table) and executes
 * plans on the objects. Every operation starts from this object.
 * This class is designed to be single-threaded.
 */
// TODO(evanj): Used by JNI so must be exported. Remove when we only one .so
class __attribute__((visibility("default"))) VoltDBEngine {
    public:
        /** Constructor for test code: this does not enable JNI callbacks. */
        VoltDBEngine() :
          m_currentUndoQuantum(NULL),
          m_catalogVersion(0),
          m_staticParams(MAX_PARAM_COUNT),
          m_currentOutputDepId(-1),
          m_currentInputDepId(-1),
          m_isELEnabled(false),
          m_numResultDependencies(0),
          m_templateSingleLongTable(NULL),
          m_topend(NULL),
          m_logProxy(NULL),
          m_logManager(new LogManager(new StdoutLogProxy())),
          m_ARIESEnabled(false)
        {
            m_currentUndoQuantum = new DummyUndoQuantum();

        }

        VoltDBEngine(Topend *topend, LogProxy *logProxy);
	
        bool initialize(
                int32_t clusterIndex,
                int32_t siteId,
                int32_t partitionId,
                int32_t hostId,
                std::string hostname);
        virtual ~VoltDBEngine();

        inline int32_t getClusterIndex() const { return m_clusterIndex; }
        inline int32_t getSiteId() const { return m_siteId; }
        inline int32_t getPartitionId() const { return m_partitionId; }

        // ------------------------------------------------------------------
        // OBJECT ACCESS FUNCTIONS
        // ------------------------------------------------------------------
        catalog::Catalog *getCatalog() const;

        Table* getTable(int32_t tableId) const;
        Table* getTable(std::string name) const;
        // Serializes table_id to out. Returns true if successful.
        bool serializeTable(int32_t tableId, SerializeOutput* out) const;

        // -------------------------------------------------
        // Execution Functions
        // -------------------------------------------------
        int executeQuery(int64_t planfragmentId, int32_t outputDependencyId, int32_t inputDependencyId,
                         const NValueArray &params, int64_t txnId, int64_t lastCommittedTxnId, bool first, bool last);
        int executePlanFragment(std::string fragmentString, int32_t outputDependencyId, int32_t inputDependencyId,
                                int64_t txnId, int64_t lastCommittedTxnId);

        inline int getUsedParamcnt() const { return m_usedParamcnt;}
        inline void setUsedParamcnt(int usedParamcnt) { m_usedParamcnt = usedParamcnt;}


        // Created to transition existing unit tests to context abstraction.
        // If using this somewhere new, consider if you're being lazy.
        ExecutorContext *getExecutorContext();

        // -------------------------------------------------
        // Dependency Transfer Functions
        // -------------------------------------------------
        bool send(Table* dependency);
        int loadNextDependency(Table* destination);

        // -------------------------------------------------
        // Catalog Functions
        // -------------------------------------------------
        bool loadCatalog(const std::string &catalogPayload);
        bool updateCatalog(const std::string &catalogPayload, int catalogVersion);
        bool processCatalogAdditions(bool addAll);
        bool processCatalogDeletes();
        bool rebuildPlanFragmentCollections();
        bool rebuildTableCollections();


        /**
        * Load table data into a persistent table specified by the tableId parameter.
        * This must be called at most only once before any data is loaded in to the table.
        */
        bool loadTable(bool allowExport, int32_t tableId,
                       ReferenceSerializeInput &serializeIn,
                       int64_t txnId, int64_t lastCommittedTxnId);

        // ARIES
        bool loadTable(PersistentTable *table,
                               ReferenceSerializeInput &serializeIn,
                               int64_t txnId, int64_t lastCommittedTxnId,
                               bool isExecutionNormal);

        void resetReusedResultOutputBuffer(const size_t headerSize = 0);
        inline ReferenceSerializeOutput* getResultOutputSerializer() { return &m_resultOutput; }
        inline ReferenceSerializeOutput* getExceptionOutputSerializer() { return &m_exceptionOutput; }

        void setBuffers(char *parameterBuffer, int parameterBuffercapacity,
                char *resultBuffer, int resultBufferCapacity,
                char *exceptionBuffer, int exceptionBufferCapacity);

        void setBuffers(char *parameter_buffer, int m_parameterBuffercapacity,
                char *resultBuffer, int resultBufferCapacity,
                char *exceptionBuffer, int exceptionBufferCapacity,
                char *arieslogBuffer, int arieslogBufferCapacity);
        inline const char* getParameterBuffer() const { return m_parameterBuffer;}
        /** Returns the size of buffer for passing parameters to EE. */
        inline int getParameterBufferCapacity() const { return m_parameterBufferCapacity;}

        /**
         * Retrieves the size in bytes of the data that has been placed in the reused result buffer
         */
        int getResultsSize() const;

        /** Returns the buffer for receiving result tables from EE. */
        inline char* getReusedResultBuffer() const { return m_reusedResultBuffer;}
        /** Returns the size of buffer for receiving result tables from EE. */
        inline int getReusedResultBufferCapacity() const { return m_reusedResultCapacity;}

        NValueArray& getParameterContainer() { return m_staticParams; }
        int64_t* getBatchFragmentIdsContainer() { return m_batchFragmentIdsContainer; }
        /** PAVLO **/
        int32_t* getBatchInputDepIdsContainer() { return m_batchInputDepIdsContainer; }
        int32_t* getBatchOutputDepIdsContainer() { return m_batchOutputDepIdsContainer; }
        /** PAVLO **/

        /** are we sending tuples to another database? */
        bool isELEnabled() { return m_isELEnabled; }

        /** check if this value hashes to the local partition */
        bool isLocalSite(const NValue& value);

        // -------------------------------------------------
        // Non-transactional work methods
        // -------------------------------------------------

        /** Perform once per second, non-transactional work. */
        void tick(int64_t timeInMillis, int64_t lastCommittedTxnId);

        /** flush active work (like EL buffers) */
        void quiesce(int64_t lastCommittedTxnId);

        // -------------------------------------------------
        // Save and Restore Table to/from disk functions
        // -------------------------------------------------

        /**
         * Save the table specified by catalog id tableId to the
         * absolute path saveFilePath
         *
         * @param tableGuid the GUID of the table in the catalog
         * @param saveFilePath the full path of the desired savefile
         * @return true if successful, false if save failed
         */
        bool saveTableToDisk(int32_t clusterId, int32_t databaseId, int32_t tableId, std::string saveFilePath);

        /**
         * Restore the table from the absolute path saveFilePath
         *
         * @param restoreFilePath the full path of the file with the
         * table to restore
         * @return true if successful, false if save failed
         */
        bool restoreTableFromDisk(std::string restoreFilePath);

        // -------------------------------------------------
        // READ/WRITE SET TRACKING FUNCTIONS
        // -------------------------------------------------
        void trackingEnable(int64_t txnId);
        void trackingFinish(int64_t txnId);
//         std::vector<std::string> trackingTablesRead(int64_t txnId);
//         std::vector<std::string> trackingTablesWritten(int64_t txnId);
        int trackingTupleSet(int64_t txnId, bool writes);
        
        // -------------------------------------------------
        // ANTI-CACHE FUNCTIONS
        // -------------------------------------------------
        void antiCacheInitialize(std::string dbDir, long blockSize) const;
#ifdef ANTICACHE
	int antiCacheReadBlocks(int32_t tableId, int numBlocks, int16_t blockIds[], int32_t tupleOffsets[]);
	int antiCacheEvictBlock(int32_t tableId, long blockSize, int numBlocks);
	int antiCacheMergeBlocks(int32_t tableId);
#endif
        
        // -------------------------------------------------
        // STORAGE MMAP
        // -------------------------------------------------
        void MMAPInitialize(std::string dbDir, long mapSize, long syncFrequency ) const;


        // ARIES
        void ARIESInitialize(std::string dbDir, std::string logFile) ;

        std::string getARIESDir(){
        	return m_ARIESDir;
        }

        void setARIESDir(std::string dbDir){
        	m_ARIESDir = dbDir;
        }

        std::string getARIESFile() {
        	return m_ARIESFile;
        }

        void setARIESFile(std::string logFile) {
        	m_ARIESFile = logFile;
        }

        bool isARIESEnabled() {
        	return m_ARIESEnabled;
        }

        void setARIESEnabled(bool status) {
        	m_ARIESEnabled = status;
        }


        // -------------------------------------------------
        // Debug functions
        // -------------------------------------------------
        std::string debug(void) const;

        /** Counts tuples modified by a plan fragment */
        int64_t m_tuplesModified;
        /** True if any fragments in a batch have modified any tuples */
        bool m_dirtyFragmentBatch;

        std::string m_stmtName;
        std::string m_fragName;

        std::map<std::string, int*> m_indexUsage;

        // -------------------------------------------------
        // Statistics functions
        // -------------------------------------------------
        voltdb::StatsAgent& getStatsManager();

        /**
         * Retrieve a set of statistics and place them into the result buffer as a set of VoltTables.
         * @param selector StatisticsSelectorType indicating what set of statistics should be retrieved
         * @param locators Integer identifiers specifying what subset of possible statistical sources should be polled. Probably a CatalogId
         *                 Can be NULL in which case all possible sources for the selector should be included.
         * @param numLocators Size of locators array.
         * @param interval Whether to return counters since the beginning or since the last time this was called
         * @param Timestamp to embed in each row
         * @return Number of result tables, 0 on no results, -1 on failure.
         */
        int getStats(
                int selector,
                int locators[],
                int numLocators,
                bool interval,
                int64_t now);

        inline Pool* getStringPool() { return &m_stringPool; }

        inline LogManager* getLogManager() {
            return m_logManager;
        }

        inline void setUndoToken(int64_t nextUndoToken) {
            if (nextUndoToken == INT64_MAX) { return; }
            if (m_currentUndoQuantum != NULL && m_currentUndoQuantum->isDummy()) {
                //std::cout << "Deleting dummy undo quantum " << std::endl;
                delete m_currentUndoQuantum;
                m_currentUndoQuantum = NULL;
            }
            if (m_currentUndoQuantum != NULL) {
                #ifdef VOLT_INFO_ENABLED
                if (nextUndoToken < m_currentUndoQuantum->getUndoToken()) {
                    VOLT_ERROR("nextUndoToken[%ld] is greater than m_currentUndoQuantum[%ld]",
                               nextUndoToken, m_currentUndoQuantum->getUndoToken());
                }
                #endif
                assert(nextUndoToken >= m_currentUndoQuantum->getUndoToken());
                if (m_currentUndoQuantum->getUndoToken() == nextUndoToken) {
                    return;
                }
            }
            m_currentUndoQuantum = m_undoLog.generateUndoQuantum(nextUndoToken);
        }

        inline void releaseUndoToken(int64_t undoToken);

        inline void undoUndoToken(int64_t undoToken) {
            if (m_currentUndoQuantum != NULL && m_currentUndoQuantum->isDummy()) {
                return;
            }
            VOLT_TRACE("Undoing Buffer Token %ld at partition %d", undoToken, m_partitionId);
            m_undoLog.undo(undoToken);
            m_currentUndoQuantum = NULL;
        }

        inline voltdb::UndoQuantum* getCurrentUndoQuantum() { return m_currentUndoQuantum; }

        inline Topend* getTopend() { return m_topend; }

        /**
         * Get a unique id for a plan fragment by munging the indices of it's parents
         * and grandparents in the catalog.
         */
        static int64_t uniqueIdForFragment(catalog::PlanFragment *frag);

        /**
         * Activate a table stream of the specified type for the specified table.
         * Returns true on success and false on failure
         */
        bool activateTableStream(const CatalogId tableId, const TableStreamType streamType);

        /**
         * Serialize more tuples from the specified table that has an active stream of the specified type
         * Returns the number of bytes worth of tuple data serialized or 0 if there are no more.
         * Returns -1 if the table is not in COW mode. The table continues to be in COW (although no copies are made)
         * after all tuples have been serialize until the last call to cowSerializeMore which returns 0 (and deletes
         * the COW context). Further calls will return -1
         */
        int tableStreamSerializeMore(
                ReferenceSerializeOutput *out,
                CatalogId tableId,
                const TableStreamType streamType);

        /*
         * Apply the updates in a recovery message.
         */
        void processRecoveryMessage(RecoveryProtoMsg *message);

#ifdef ARIES
        // do aries recovery - startup work.
        void doAriesRecovery(char *logData, size_t length, int64_t replay_txnid);

        char* readAriesLogForReplay(int64_t* sizes);

        void freePointerToReplayLog(char *logData);

        void writeToAriesLogBuffer(const char *data, size_t size);

        size_t getArieslogBufferLength();

        void rewindArieslogBuffer();
#endif

        /**
         * Perform an action on behalf of Export.
         *
         * @param ackAction whether or not this action include a
         * release for stream octets
         * @param pollAction whether or not this action requests the
         * next buffer of unpolled octets
         * @param if ackAction is true, the stream offset being released
         * @param if syncAction is true, the stream offset being set for a table
         * @param the catalog version qualified id of the table to which this action applies
         * @return the universal offset for any poll results (results
         * returned separatedly via QueryResults buffer)
         */
        long exportAction(bool ackAction, bool pollAction, bool resetAction, bool syncAction,
                          int64_t ackOffset, int64_t seqNo, int64_t tableId);

        /**
         * Retrieve a hash code for the specified table
         */
        size_t tableHashCode(int32_t tableId);

    protected:
        /*
         * Get the list of persistent table Ids by inspecting the catalog.
         */
        std::vector<int32_t> getPersistentTableIds();
        std::string getClusterNameFromTable(voltdb::Table *table);
        std::string getDatabaseNameFromTable(voltdb::Table *table);

        // -------------------------------------------------
        // Initialization Functions
        // -------------------------------------------------
        bool initPlanFragment(const int64_t fragId, const std::string planNodeTree);
        bool initPlanNode(const int64_t fragId, AbstractPlanNode* node, int* tempTableMemoryInBytes);
        bool initCluster();
        bool initMaterializedViews(bool addAll);
        bool updateCatalogDatabaseReference();

        void printReport();


        /**
         * Keep a list of executors for runtime - intentionally near the top of VoltDBEngine
         */
        struct ExecutorVector {
            std::vector<AbstractExecutor*> list;
            int tempTableMemoryInBytes;
        };
        std::map<int64_t, boost::shared_ptr<ExecutorVector> > m_executorMap;

        voltdb::UndoLog m_undoLog;
        voltdb::UndoQuantum *m_currentUndoQuantum;

        // -------------------------------------------------
        // Data Members
        // -------------------------------------------------
        int32_t m_siteId;
        int32_t m_partitionId;
        int32_t m_clusterIndex;
        int m_totalPartitions;
        size_t m_startOfResultBuffer;

        /*
         * Catalog delegates hashed by path.
         */
        std::map<std::string, CatalogDelegate*> m_catalogDelegates;

        // map catalog table id to table pointers
        std::map<int32_t, Table*> m_tables;

        // map catalog table name to table pointers
        std::map<std::string, Table*> m_tablesByName;

        /*
         * Map of catalog table ids to snapshotting tables.
         * Note that these tableIds are the ids when the snapshot
         * was initiated. The snapshot processor in Java does not
         * update tableIds when the catalog changes. The point of
         * reference, therefore, is consistently the catalog at
         * the point of snapshot initiation. It is always invalid
         * to try to map this tableId back to catalog::Table via
         * the catalog, at least w/o comparing table names.
         */
        std::map<int32_t, Table*> m_snapshottingTables;

        /*
         * Map of catalog ids to exporting tables.
         */
        std::map<int64_t, Table*> m_exportingTables;

        /**
         * System Catalog.
        */
        boost::shared_ptr<catalog::Catalog> m_catalog;
        int m_catalogVersion;
        catalog::Database *m_database;

        /** reused parameter container. */
        NValueArray m_staticParams;
        /** TODO : should be passed as execute() parameter..*/
        int m_usedParamcnt;

        /** buffer object for result tables. set when the result table is sent out to localsite. */
        ReferenceSerializeOutput m_resultOutput;

        // ARIES
        /** buffer object for aries log generated by the EE */
        FallbackSerializeOutput m_arieslogOutput;

        /** buffer object for exceptions generated by the EE **/
        ReferenceSerializeOutput m_exceptionOutput;

        /** buffer object to pass parameters to EE. */
        const char* m_parameterBuffer;
        /** size of parameter_buffer. */
        int m_parameterBufferCapacity;

        char *m_exceptionBuffer;

        int m_exceptionBufferCapacity;

        /** buffer object to receive result tables from EE. */
        char* m_reusedResultBuffer;
        /** size of reused_result_buffer. */
        int m_reusedResultCapacity;

        // ARIES
        /** buffer object to store aries log */
        char* m_arieslogBuffer;

        int m_arieslogBufferCapacity;

        size_t m_ariesWriteOffset;

        std::string m_ARIESDir ;
        std::string m_ARIESFile ;

        bool m_isRecovering;	// are we currently recovering?

        int64_t m_batchFragmentIdsContainer[MAX_BATCH_COUNT];
        /** PAVLO **/
        int32_t m_batchInputDepIdsContainer[MAX_BATCH_COUNT];
        int32_t m_batchOutputDepIdsContainer[MAX_BATCH_COUNT];
        /** PAVLO **/

        /** number of plan fragments executed so far */
        int m_pfCount;

        // used for sending and recieving deps
        // set by the executeQuery / executeFrag type methods
        int m_currentOutputDepId;
        int m_currentInputDepId;

        /** EL subsystem on/off, pulled from catalog */
        bool m_isELEnabled;

        /** Stats manager for this execution engine **/
        voltdb::StatsAgent m_statsManager;

        /*
         * Pool for short lived strings that will not live past the return back to Java.
         */
        Pool m_stringPool;

        /*
         * When executing a plan fragment this is set to the number of result dependencies
         * that have been serialized into the m_resultOutput
         */
        int32_t m_numResultDependencies;

        /*
         * Cache plan node fragments in order to allow for deletion.
         */
        std::vector<PlanNodeFragment*> m_planFragments;

        char *m_templateSingleLongTable;

        // depid + table size + status code + header size + column count + column type
        // + column name + tuple count + first row size + modified tuples
        const static int m_templateSingleLongTableSize = 4 + 4 + 1 + 4 + 2 + 1 + 4 + 4 + 4 + 8;

        Topend *m_topend;

        // For data from engine that must be shared/distributed to
        // other components. (Components MUST NOT depend on VoltDBEngine.h).
        ExecutorContext *m_executorContext;

        DefaultTupleSerializer m_tupleSerializer;

        LogProxy* m_logProxy;

        LogManager* m_logManager;

        bool m_ARIESEnabled ;

    private:
        ThreadLocalPool m_tlPool;

};

inline void VoltDBEngine::resetReusedResultOutputBuffer(const size_t headerSize) {
    m_resultOutput.initializeWithPosition(m_reusedResultBuffer, m_reusedResultCapacity, headerSize);
    m_exceptionOutput.initializeWithPosition(m_exceptionBuffer, m_exceptionBufferCapacity, headerSize);
    *reinterpret_cast<int32_t*>(m_exceptionBuffer) = voltdb::VOLT_EE_EXCEPTION_TYPE_NONE;

    //#ifdef ARIES
	// XXX: I don't see function being called anywhere else beside when exceptions are thrown,
	// not sure if this initialization even ever happens unless a serializeexception is thrown.
	m_arieslogOutput.initializeWithPosition(m_arieslogBuffer,
			m_arieslogBufferCapacity, headerSize);
	//#endif
}

void VoltDBEngine::releaseUndoToken(int64_t undoToken){
  if (m_currentUndoQuantum != NULL && m_currentUndoQuantum->isDummy()) {
    return;
  }

#ifdef STORAGE_MMAP
  if(m_executorContext->isMMAPEnabled()){    
      for (std::map<int32_t, Table*>::iterator m_tables_itr = m_tables.begin() ; m_tables_itr != m_tables.end() ; ++m_tables_itr){
          Table* table = m_tables_itr->second;

          // Fix Group Commit Interval       
          int64_t m_groupCommitInterval = m_executorContext->getMMAPSyncFrequency() ;
          VOLT_WARN("Sync Frequency: %ld", m_groupCommitInterval);

          if(m_currentUndoQuantum != NULL && m_currentUndoQuantum->getUndoToken() % m_groupCommitInterval == 0){
              VOLT_WARN("Undo Token: %ld", m_currentUndoQuantum->getUndoToken());

              if(table != NULL){
                  //VOLT_WARN("Syncing Table %s",table->name().c_str());	  
                  /*Pool* pool = table->getPool();
                  if(pool != NULL)
                      MMAPMemoryManager* m_pool_manager = pool->getPoolManager();
                  if(m_pool_manager != NULL)
                      m_pool_manager->sync();*/

                  MMAPMemoryManager* m_data_manager = table->getDataManager();
                  if(m_data_manager != NULL)
                      m_data_manager->sync();
              }
          }
      }
  }
#endif

  if (m_currentUndoQuantum != NULL && m_currentUndoQuantum->getUndoToken() == undoToken) {
      m_currentUndoQuantum = NULL;    
  }

  VOLT_TRACE("Committing Buffer Token %ld at partition %d", undoToken, m_partitionId);
  m_undoLog.release(undoToken);
}


} // namespace voltdb

#endif // VOLTDBENGINE_H
