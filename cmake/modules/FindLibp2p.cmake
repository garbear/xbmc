# FindLibp2p
# --------
# Finds the libp2p libraries and dependencies
#
# This will define the following variables::
#
# LIBP2P_FOUND - system has libp2p
# LIBP2P_LIBRARIES - the libp2p libraries
#

find_package(libp2p REQUIRED CONFIG)

# libp2p dependencies
find_package(Boost REQUIRED filesystem program_options random)
find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)
find_package(tsl_hat_trie REQUIRED)

set(LIBP2P_FOUND 1)

# TODO: This comes from generated libp2pConfig.cmake
set(LIBP2P_LIBRARIES p2p::p2p_varint_reader p2p::p2p_message_read_writer_error p2p::p2p_message_read_writer p2p::p2p_protobuf_message_read_writer p2p::p2p_hexutil p2p::p2p_logger p2p::p2p_byteutil p2p::p2p_literals p2p::p2p_aes_provider p2p::p2p_crypto_provider p2p::p2p_ecdsa_provider p2p::p2p_ed25519_provider p2p::p2p_hmac_provider p2p::p2p_key_marshaller p2p::p2p_key_validator p2p::p2p_keys_proto p2p::p2p_random_generator p2p::p2p_rsa_provider p2p::p2p_secp256k1_provider p2p::p2p_sha p2p::p2p_crypto_error p2p::p2p_crypto_key p2p::p2p_crypto_common p2p::p2p_basic_host p2p::p2p_default_host p2p::p2p_converters p2p::p2p_multibase_codec p2p::p2p_uvarint p2p::p2p_multihash p2p::p2p_multiaddress p2p::p2p_cid p2p::p2p_yamux p2p::p2p_yamuxed_connection p2p::p2p_mplex p2p::p2p_mplexed_connection p2p::p2p_router p2p::p2p_listener_manager p2p::p2p_dialer p2p::p2p_network p2p::p2p_transport_manager p2p::p2p_connection_manager p2p::p2p_default_network p2p::p2p_inmem_address_repository p2p::p2p_inmem_key_repository p2p::p2p_inmem_protocol_repository p2p::p2p_identity_manager p2p::p2p_peer_repository p2p::p2p_address_repository p2p::p2p_peer_errors p2p::p2p_peer_id p2p::p2p_peer_address p2p::scheduler p2p::asio_scheduler p2p::subscription p2p::p2p_protocol_echo p2p::p2p_identify_proto p2p::p2p_identify p2p::p2p_ping p2p::p2p_kad_proto p2p::p2p_kad p2p::p2p_gossip_proto p2p::p2p_gossip p2p::p2p_multiselect p2p::p2p_plaintext_protobuf p2p::p2p_plaintext p2p::p2p_plaintext_exchange_message_marshaller p2p::p2p_secio_protobuf p2p::p2p_secio p2p::p2p_secio_propose_message_marshaller p2p::p2p_secio_exchange_message_marshaller p2p::p2p_security_error p2p::p2p_transport_parser p2p::p2p_upgrader p2p::p2p_upgrader_session p2p::p2p_tcp_connection p2p::p2p_tcp_listener p2p::p2p_tcp p2p::p2p)
