
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "define.h"
#include "peers.h"
#include "hashmap.h"
#include "log.h"
#include "define.h"

void umesh_peers_state_init(umesh_peers_state_t *state) {
	state->peers = umesh_peers_init();
	state->timeout = UMESH_PEERS_DEFAULT_CLEAN_INTERVAL;
}

void umesh_peers_state_deinit(umesh_peers_state_t *state) {
	umesh_peers_free(state->peers);
}
umesh_peers_t umesh_peers_init() {
	return (umesh_peers_t) hashmap_new(IEEE80211_MAC_ADDR_LEN);
}

void umesh_peers_free(umesh_peers_t peers) {
	struct umesh_peer *peer;
	map_t map = (map_t) peers;

	/* Remove all allocated peers */
	map_it_t it = hashmap_it_new(map);
	while (hashmap_it_next(it, NULL, (any_t *) &peer) == MAP_OK) {
		hashmap_it_remove(it);
		free(peer);
	}
	hashmap_it_free(it);

	/* Remove hashmap itself */
	hashmap_free(peers);
}

int umesh_peers_length(umesh_peers_t peers) {
	map_t map = (map_t) peers;
	return hashmap_length(map);
}

// static int umesh_peer_is_valid(const struct umesh_peer *peer) {
// 	return peer->step; /*!!! */
// }

 static struct umesh_peer *umesh_peer_new(const uint8_t *addr) {
	if(addr == NULL) {
		return NULL;
	}
	struct umesh_peer *peer = (struct umesh_peer *) umesh_malloc(sizeof(struct umesh_peer));
	memset(peer, 0, sizeof(struct umesh_peer));
	memcpy(peer->addr,addr,IEEE80211_MAC_ADDR_LEN );
	peer->last_update = 0;
	peer->version = 0;

	return peer;
}

struct umesh_peer *umesh_peer_add2(umesh_peers_t peers, const uint8_t *_addr, 
	umesh_peer_type_t type, umesh_identify_step_t step,
	uint64_t now, uint32_t session_id,
	uint8_t *from_random, uint8_t *to_random ) {
	int status, result;
	map_t map = (map_t) peers;
	mkey_t addr = (mkey_t) _addr;
	struct umesh_peer *peer = NULL;
	//log_info("--- ready add peer %s (%s)", ether_ntoa(&peer->addr), peer->name==NULL?"NULL":peer->name);
	status = hashmap_get(map, addr, (any_t *) &peer, 0 /* do not remove */);
	if (status == MAP_MISSING)
		peer = umesh_peer_new(_addr); /* create new entry */

    if(peer == NULL) {
		return NULL;
	}
	/* update */
	peer->type = type;
	peer->step = step;

	peer->session_id = session_id;
	peer->last_update = now;
	if(from_random != NULL) {
		memcpy(peer->from_random,from_random,UMESH_PEER_RANDOM_LEN);
	}
    if(to_random != NULL) {
		memcpy(peer->to_random,to_random,UMESH_PEER_RANDOM_LEN);
	}
	
	if (status == MAP_OK) {
		result = PEERS_UPDATE;
		goto out;
	}

	status = hashmap_put(map, (mkey_t) &peer->addr, peer);
	if (status != MAP_OK) {
		free(peer);
		return NULL;
	}
out:
	// if (!peer->is_valid && umesh_peer_is_valid(peer)) {
	// 	/* peer has turned valid */
	// 	peer->is_valid = 1;
	// 	if (cb)
	// 		cb(peer, arg);
	// }
	return peer;
}

enum peers_status umesh_peer_add(umesh_peers_t peers, const uint8_t *_addr, 
                                 uint64_t now) {
	int status, result;
	map_t map = (map_t) peers;
	mkey_t addr = (mkey_t) _addr;
	struct umesh_peer *peer;
	//log_info("--- ready add peer %s (%s)", ether_ntoa(&peer->addr), peer->name==NULL?"NULL":peer->name);
	status = hashmap_get(map, addr, (any_t *) &peer, 0 /* do not remove */);
	if (status == MAP_MISSING)
		peer = umesh_peer_new(_addr); /* create new entry */

	/* update */
	peer->last_update = now;

	if (status == MAP_OK) {
		result = PEERS_UPDATE;
		goto out;
	}

	status = hashmap_put(map, (mkey_t) &peer->addr, peer);
	if (status != MAP_OK) {
		free(peer);
		return PEERS_ERR;
	}
	result = PEERS_OK;
out:
	// if (!peer->is_valid && umesh_peer_is_valid(peer)) {
	// 	/* peer has turned valid */
	// 	peer->is_valid = 1;
	// 	if (cb)
	// 		cb(peer, arg);
	// }
	return result;
}

 enum peers_status umesh_peer_remove(umesh_peers_t peers, const uint8_t *_addr, umesh_peer_cb cb, void *arg) {
	int status;
	map_t map = (map_t) peers;
	mkey_t addr = (mkey_t) _addr;
	struct umesh_peer *peer;
	status = hashmap_get(map, addr, (any_t *) &peer, 1 /* remove */);
	if (status == MAP_MISSING)
		return UMESH_ERR_PEER_MISSING;
	//if (peer->is_valid) {
		//log_info("remove peer %s (%s)", ether_ntoa(&peer->addr));
		if (cb)
			cb(peer, arg);
	//}
	free(peer);
	return 0;
}

int umesh_peer_get(umesh_peers_t peers, const uint8_t *_addr, struct umesh_peer **peer) {
	int status;
	map_t map = (map_t) peers;
	mkey_t addr = (mkey_t) _addr;
	status = hashmap_get(map, addr, (any_t *) peer, 0 /* keep */);
	if (status == MAP_MISSING)
		return UMESH_ERR_PEER_MISSING;
	return 0;
}

int umesh_peer_print(const struct umesh_peer *peer, char *str, uint32_t len) {
	char *cur = str, *const end = str + len;
	cur += snprintf(cur, end - cur, "%02x:%02x:%02x:%02x:%02x:%02x ", peer->addr[0], peer->addr[1], peer->addr[2], peer->addr[3], peer->addr[4], peer->addr[5]);
	return end - cur;
}

int umesh_peers_print(umesh_peers_t peers, char *str, uint32_t len) {
	char *cur = str, *const end = str + len;
	map_t map = (map_t) peers;
	map_it_t it = hashmap_it_new(map);
	struct umesh_peer *peer;

	while (hashmap_it_next(it, NULL, (any_t *) &peer) == MAP_OK) {
		cur += umesh_peer_print(peer, cur, end - cur);
		cur += snprintf(cur, end - cur, "\n");
	}

	hashmap_it_free(it);
	return end - cur;
}

void umesh_peers_remove(umesh_peers_t peers, uint64_t before, umesh_peer_cb cb, void *arg) {
	map_t map = (map_t) peers;
	map_it_t it = hashmap_it_new(map);
	struct umesh_peer *peer;

	while (hashmap_it_next(it, NULL, (any_t *) &peer) == MAP_OK) {
		if (peer->last_update < before) {
			//if (peer->is_valid) {
			//	log_info("remove peer %s (%s)", ether_ntoa(&peer->addr), peer->name);
				if (cb)
					cb(peer, arg);
			//}
			hashmap_it_remove(it);
			free(peer);
		}
	}
	hashmap_it_free(it);
}

umesh_peers_it_t umesh_peers_it_new(umesh_peers_t in) {
	return (umesh_peers_it_t) hashmap_it_new((map_t) in);
}

enum peers_status umesh_peers_it_next(umesh_peers_it_t it, struct umesh_peer **peer) {
	int s = hashmap_it_next((map_it_t) it, 0, (any_t *) peer);
	if (s == MAP_OK)
		return PEERS_OK;
	else
		return PEERS_MISSING;
}

enum peers_status umesh_peers_it_remove(umesh_peers_it_t it) {
	int s = hashmap_it_remove((map_it_t) it);
	if (s == MAP_OK)
		return PEERS_OK;
	else
		return PEERS_MISSING;
}

void umesh_peers_it_free(umesh_peers_it_t it) {
	hashmap_it_free((map_it_t) it);
}