# FindFilecoin
# --------
# Finds the Filecoin libraries and dependencies
#
# This will define the following variables::
#
# FILECOIN_FOUND - system has Filecoin
# FILECOIN_LIBRARIES - the Filecoin libraries
#

find_package(filecoin REQUIRED CONFIG)

# Filecoin dependencies
find_package(Boost REQUIRED date_time filesystem random system)
find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)
find_package(tsl_hat_trie REQUIRED)
find_package(leveldb REQUIRED)

set(FILECOIN_FOUND 1)

# TODO: This comes from generated filecoinConfig.cmake
set(FILECOIN_LIBRARIES
  filecoin::filecoin_array
  filecoin::filecoin_balance_table_hamt
  filecoin::filecoin_multimap
  filecoin::filecoin_message_pool
  filecoin::filecoin_block_producer
  filecoin::filecoin_block_validator
  filecoin::filecoin_weight_calculator
  filecoin::filecoin_clock
  filecoin::filecoin_cbor
  filecoin::filecoin_rle_plus_codec
  filecoin::filecoin_hexutil
  filecoin::filecoin_blob
  filecoin::filecoin_outcome
  filecoin::filecoin_buffer
  filecoin::filecoin_logger
  filecoin::filecoin_blake2
  filecoin::filecoin_bls_provider
  filecoin::filecoin_murmur
  filecoin::filecoin_randomness_provider
  filecoin::filecoin_vrf_provider
  filecoin::filecoin_signature
  filecoin::filecoin_address
  filecoin::filecoin_block
  filecoin::filecoin_chain
  filecoin::filecoin_chain_epoch_codec
  filecoin::filecoin_cid
  filecoin::filecoin_comm_cid
  filecoin::filecoin_piece
  filecoin::filecoin_rle_bitset
  filecoin::filecoin_tickets
  filecoin::filecoin_tipset
  filecoin::filecoin_amt
  filecoin::filecoin_chain_store
  filecoin::filecoin_datastore_key
  filecoin::filecoin_chain_data_store
  filecoin::filecoin_config
  filecoin::filecoin_filestore
  filecoin::filecoin_hamt
  filecoin::filecoin_in_memory_storage
  filecoin::filecoin_ipfs_datastore_in_memory
  filecoin::filecoin_ipfs_datastore_leveldb
  filecoin::filecoin_ipfs_blockservice
  filecoin::filecoin_ipfs_merkledag_service
  filecoin::filecoin_keystore
  filecoin::filecoin_leveldb
  filecoin::filecoin_repository
  filecoin::filecoin_filesystem_repository
  filecoin::filecoin_in_memory_repository
  filecoin::filecoin_fslock
  filecoin::filecoin_power_table
  filecoin::filecoin_power_table_hamt
  filecoin::filecoin_account_actor
  filecoin::filecoin_cron_actor
  filecoin::filecoin_init_actor
  filecoin::filecoin_miner_actor
  filecoin::filecoin_multisig_actor
  filecoin::filecoin_payment_channel_actor
  filecoin::filecoin_reward_actor
  filecoin::filecoin_builtin_shared
  filecoin::filecoin_storage_power_actor
  filecoin::filecoin_actor
  filecoin::filecoin_exit_code
  filecoin::filecoin_indices
  filecoin::filecoin_interpreter
  filecoin::filecoin_runtime
  filecoin::filecoin_state_tree
  filecoin::filecoin_message
)
