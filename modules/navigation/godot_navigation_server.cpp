/*************************************************************************/
/*  godot_navigation_server.cpp                                          */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "godot_navigation_server.h"

#include "core/os/mutex.h"

#ifndef _3D_DISABLED
#include "navigation_mesh_generator.h"
#endif

/// Creates a struct for each function and a function that once called creates
/// an instance of that struct with the submitted parameters.
/// Then, that struct is stored in an array; the `sync` function consume that array.

#define COMMAND_1(F_NAME, T_0, D_0)                                 \
	struct MERGE(F_NAME, _command) : public SetCommand {            \
		T_0 d_0;                                                    \
		MERGE(F_NAME, _command)                                     \
		(T_0 p_d_0) :                                               \
				d_0(p_d_0) {}                                       \
		virtual void exec(GodotNavigationServer *server) override { \
			server->MERGE(_cmd_, F_NAME)(d_0);                      \
		}                                                           \
	};                                                              \
	void GodotNavigationServer::F_NAME(T_0 D_0) const {             \
		auto cmd = memnew(MERGE(F_NAME, _command)(                  \
				D_0));                                              \
		add_command(cmd);                                           \
	}                                                               \
	void GodotNavigationServer::MERGE(_cmd_, F_NAME)(T_0 D_0)

#define COMMAND_2(F_NAME, T_0, D_0, T_1, D_1)                       \
	struct MERGE(F_NAME, _command) : public SetCommand {            \
		T_0 d_0;                                                    \
		T_1 d_1;                                                    \
		MERGE(F_NAME, _command)                                     \
		(                                                           \
				T_0 p_d_0,                                          \
				T_1 p_d_1) :                                        \
				d_0(p_d_0),                                         \
				d_1(p_d_1) {}                                       \
		virtual void exec(GodotNavigationServer *server) override { \
			server->MERGE(_cmd_, F_NAME)(d_0, d_1);                 \
		}                                                           \
	};                                                              \
	void GodotNavigationServer::F_NAME(T_0 D_0, T_1 D_1) const {    \
		auto cmd = memnew(MERGE(F_NAME, _command)(                  \
				D_0,                                                \
				D_1));                                              \
		add_command(cmd);                                           \
	}                                                               \
	void GodotNavigationServer::MERGE(_cmd_, F_NAME)(T_0 D_0, T_1 D_1)

#define COMMAND_4(F_NAME, T_0, D_0, T_1, D_1, T_2, D_2, T_3, D_3)                  \
	struct MERGE(F_NAME, _command) : public SetCommand {                           \
		T_0 d_0;                                                                   \
		T_1 d_1;                                                                   \
		T_2 d_2;                                                                   \
		T_3 d_3;                                                                   \
		MERGE(F_NAME, _command)                                                    \
		(                                                                          \
				T_0 p_d_0,                                                         \
				T_1 p_d_1,                                                         \
				T_2 p_d_2,                                                         \
				T_3 p_d_3) :                                                       \
				d_0(p_d_0),                                                        \
				d_1(p_d_1),                                                        \
				d_2(p_d_2),                                                        \
				d_3(p_d_3) {}                                                      \
		virtual void exec(GodotNavigationServer *server) override {                \
			server->MERGE(_cmd_, F_NAME)(d_0, d_1, d_2, d_3);                      \
		}                                                                          \
	};                                                                             \
	void GodotNavigationServer::F_NAME(T_0 D_0, T_1 D_1, T_2 D_2, T_3 D_3) const { \
		auto cmd = memnew(MERGE(F_NAME, _command)(                                 \
				D_0,                                                               \
				D_1,                                                               \
				D_2,                                                               \
				D_3));                                                             \
		add_command(cmd);                                                          \
	}                                                                              \
	void GodotNavigationServer::MERGE(_cmd_, F_NAME)(T_0 D_0, T_1 D_1, T_2 D_2, T_3 D_3)

GodotNavigationServer::GodotNavigationServer() {}

GodotNavigationServer::~GodotNavigationServer() {
	flush_queries();
}

void GodotNavigationServer::add_command(SetCommand *command) const {
	GodotNavigationServer *mut_this = const_cast<GodotNavigationServer *>(this);
	{
		MutexLock lock(commands_mutex);
		mut_this->commands.push_back(command);
	}
}

TypedArray<RID> GodotNavigationServer::get_maps() const {
	TypedArray<RID> all_map_rids;
	List<RID> maps_owned;
	map_owner.get_owned_list(&maps_owned);
	if (maps_owned.size()) {
		for (const RID &E : maps_owned) {
			all_map_rids.push_back(E);
		}
	}
	return all_map_rids;
}

RID GodotNavigationServer::map_create() const {
	GodotNavigationServer *mut_this = const_cast<GodotNavigationServer *>(this);
	MutexLock lock(mut_this->operations_mutex);
	RID rid = map_owner.make_rid();
	NavMap *space = map_owner.get_or_null(rid);
	space->set_self(rid);
	return rid;
}

COMMAND_2(map_set_active, RID, p_map, bool, p_active) {
	NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND(map == nullptr);

	if (p_active) {
		if (!map_is_active(p_map)) {
			active_maps.push_back(map);
			active_maps_update_id.push_back(map->get_map_update_id());
		}
	} else {
		int map_index = active_maps.find(map);
		ERR_FAIL_COND(map_index < 0);
		active_maps.remove_at(map_index);
		active_maps_update_id.remove_at(map_index);
	}
}

bool GodotNavigationServer::map_is_active(RID p_map) const {
	NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, false);

	return active_maps.find(map) >= 0;
}

COMMAND_2(map_set_up, RID, p_map, Vector3, p_up) {
	NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND(map == nullptr);

	map->set_up(p_up);
}

Vector3 GodotNavigationServer::map_get_up(RID p_map) const {
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, Vector3());

	return map->get_up();
}

COMMAND_2(map_set_cell_size, RID, p_map, real_t, p_cell_size) {
	NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND(map == nullptr);

	map->set_cell_size(p_cell_size);
}

real_t GodotNavigationServer::map_get_cell_size(RID p_map) const {
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, 0);

	return map->get_cell_size();
}

COMMAND_2(map_set_edge_connection_margin, RID, p_map, real_t, p_connection_margin) {
	NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND(map == nullptr);

	map->set_edge_connection_margin(p_connection_margin);
}

real_t GodotNavigationServer::map_get_edge_connection_margin(RID p_map) const {
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, 0);

	return map->get_edge_connection_margin();
}

COMMAND_2(map_set_link_connection_radius, RID, p_map, real_t, p_connection_radius) {
	NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND(map == nullptr);

	map->set_link_connection_radius(p_connection_radius);
}

real_t GodotNavigationServer::map_get_link_connection_radius(RID p_map) const {
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, 0);

	return map->get_link_connection_radius();
}

Vector<Vector3> GodotNavigationServer::map_get_path(RID p_map, Vector3 p_origin, Vector3 p_destination, bool p_optimize, uint32_t p_navigation_layers) const {
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, Vector<Vector3>());

	return map->get_path(p_origin, p_destination, p_optimize, p_navigation_layers);
}

Vector3 GodotNavigationServer::map_get_closest_point_to_segment(RID p_map, const Vector3 &p_from, const Vector3 &p_to, const bool p_use_collision) const {
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, Vector3());

	return map->get_closest_point_to_segment(p_from, p_to, p_use_collision);
}

Vector3 GodotNavigationServer::map_get_closest_point(RID p_map, const Vector3 &p_point) const {
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, Vector3());

	return map->get_closest_point(p_point);
}

Vector3 GodotNavigationServer::map_get_closest_point_normal(RID p_map, const Vector3 &p_point) const {
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, Vector3());

	return map->get_closest_point_normal(p_point);
}

RID GodotNavigationServer::map_get_closest_point_owner(RID p_map, const Vector3 &p_point) const {
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, RID());

	return map->get_closest_point_owner(p_point);
}

TypedArray<RID> GodotNavigationServer::map_get_links(RID p_map) const {
	TypedArray<RID> link_rids;
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, link_rids);

	const LocalVector<NavLink *> links = map->get_links();
	link_rids.resize(links.size());

	for (uint32_t i = 0; i < links.size(); i++) {
		link_rids[i] = links[i]->get_self();
	}
	return link_rids;
}

TypedArray<RID> GodotNavigationServer::map_get_regions(RID p_map) const {
	TypedArray<RID> regions_rids;
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, regions_rids);
	const LocalVector<NavRegion *> regions = map->get_regions();
	regions_rids.resize(regions.size());
	for (uint32_t i = 0; i < regions.size(); i++) {
		regions_rids[i] = regions[i]->get_self();
	}
	return regions_rids;
}

TypedArray<RID> GodotNavigationServer::map_get_agents(RID p_map) const {
	TypedArray<RID> agents_rids;
	const NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND_V(map == nullptr, agents_rids);
	const LocalVector<RvoAgent *> agents = map->get_agents();
	agents_rids.resize(agents.size());
	for (uint32_t i = 0; i < agents.size(); i++) {
		agents_rids[i] = agents[i]->get_self();
	}
	return agents_rids;
}

RID GodotNavigationServer::region_get_map(RID p_region) const {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND_V(region == nullptr, RID());
	if (region->get_map()) {
		return region->get_map()->get_self();
	}
	return RID();
}

RID GodotNavigationServer::agent_get_map(RID p_agent) const {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND_V(agent == nullptr, RID());
	if (agent->get_map()) {
		return agent->get_map()->get_self();
	}
	return RID();
}

RID GodotNavigationServer::region_create() const {
	GodotNavigationServer *mut_this = const_cast<GodotNavigationServer *>(this);
	MutexLock lock(mut_this->operations_mutex);
	RID rid = region_owner.make_rid();
	NavRegion *reg = region_owner.get_or_null(rid);
	reg->set_self(rid);
	return rid;
}

COMMAND_2(region_set_map, RID, p_region, RID, p_map) {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND(region == nullptr);

	if (region->get_map() != nullptr) {
		if (region->get_map()->get_self() == p_map) {
			return; // Pointless
		}

		region->get_map()->remove_region(region);
		region->set_map(nullptr);
	}

	if (p_map.is_valid()) {
		NavMap *map = map_owner.get_or_null(p_map);
		ERR_FAIL_COND(map == nullptr);

		map->add_region(region);
		region->set_map(map);
	}
}

COMMAND_2(region_set_transform, RID, p_region, Transform3D, p_transform) {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND(region == nullptr);

	region->set_transform(p_transform);
}

COMMAND_2(region_set_enter_cost, RID, p_region, real_t, p_enter_cost) {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND(region == nullptr);
	ERR_FAIL_COND(p_enter_cost < 0.0);

	region->set_enter_cost(p_enter_cost);
}

real_t GodotNavigationServer::region_get_enter_cost(RID p_region) const {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND_V(region == nullptr, 0);

	return region->get_enter_cost();
}

COMMAND_2(region_set_travel_cost, RID, p_region, real_t, p_travel_cost) {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND(region == nullptr);
	ERR_FAIL_COND(p_travel_cost < 0.0);

	region->set_travel_cost(p_travel_cost);
}

real_t GodotNavigationServer::region_get_travel_cost(RID p_region) const {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND_V(region == nullptr, 0);

	return region->get_travel_cost();
}

COMMAND_2(region_set_owner_id, RID, p_region, ObjectID, p_owner_id) {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND(region == nullptr);

	region->set_owner_id(p_owner_id);
}

ObjectID GodotNavigationServer::region_get_owner_id(RID p_region) const {
	const NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND_V(region == nullptr, ObjectID());

	return region->get_owner_id();
}

bool GodotNavigationServer::region_owns_point(RID p_region, const Vector3 &p_point) const {
	const NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND_V(region == nullptr, false);
	if (region->get_map()) {
		RID closest_point_owner = map_get_closest_point_owner(region->get_map()->get_self(), p_point);
		return closest_point_owner == region->get_self();
	}
	return false;
}

COMMAND_2(region_set_navigation_layers, RID, p_region, uint32_t, p_navigation_layers) {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND(region == nullptr);

	region->set_navigation_layers(p_navigation_layers);
}

uint32_t GodotNavigationServer::region_get_navigation_layers(RID p_region) const {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND_V(region == nullptr, 0);

	return region->get_navigation_layers();
}

COMMAND_2(region_set_navigation_mesh, RID, p_region, Ref<NavigationMesh>, p_navigation_mesh) {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND(region == nullptr);

	region->set_mesh(p_navigation_mesh);
}

void GodotNavigationServer::region_bake_navigation_mesh(Ref<NavigationMesh> p_navigation_mesh, Node *p_root_node) const {
	ERR_FAIL_COND(p_navigation_mesh.is_null());
	ERR_FAIL_COND(p_root_node == nullptr);

#ifndef _3D_DISABLED
	NavigationMeshGenerator::get_singleton()->clear(p_navigation_mesh);
	NavigationMeshGenerator::get_singleton()->bake(p_navigation_mesh, p_root_node);
#endif
}

int GodotNavigationServer::region_get_connections_count(RID p_region) const {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND_V(!region, 0);

	return region->get_connections_count();
}

Vector3 GodotNavigationServer::region_get_connection_pathway_start(RID p_region, int p_connection_id) const {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND_V(!region, Vector3());

	return region->get_connection_pathway_start(p_connection_id);
}

Vector3 GodotNavigationServer::region_get_connection_pathway_end(RID p_region, int p_connection_id) const {
	NavRegion *region = region_owner.get_or_null(p_region);
	ERR_FAIL_COND_V(!region, Vector3());

	return region->get_connection_pathway_end(p_connection_id);
}

RID GodotNavigationServer::link_create() const {
	GodotNavigationServer *mut_this = const_cast<GodotNavigationServer *>(this);
	MutexLock lock(mut_this->operations_mutex);
	RID rid = link_owner.make_rid();
	NavLink *link = link_owner.get_or_null(rid);
	link->set_self(rid);
	return rid;
}

COMMAND_2(link_set_map, RID, p_link, RID, p_map) {
	NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND(link == nullptr);

	if (link->get_map() != nullptr) {
		if (link->get_map()->get_self() == p_map) {
			return; // Pointless
		}

		link->get_map()->remove_link(link);
		link->set_map(nullptr);
	}

	if (p_map.is_valid()) {
		NavMap *map = map_owner.get_or_null(p_map);
		ERR_FAIL_COND(map == nullptr);

		map->add_link(link);
		link->set_map(map);
	}
}

RID GodotNavigationServer::link_get_map(const RID p_link) const {
	const NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND_V(link == nullptr, RID());

	if (link->get_map()) {
		return link->get_map()->get_self();
	}
	return RID();
}

COMMAND_2(link_set_bidirectional, RID, p_link, bool, p_bidirectional) {
	NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND(link == nullptr);

	link->set_bidirectional(p_bidirectional);
}

bool GodotNavigationServer::link_is_bidirectional(RID p_link) const {
	const NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND_V(link == nullptr, false);

	return link->is_bidirectional();
}

COMMAND_2(link_set_navigation_layers, RID, p_link, uint32_t, p_navigation_layers) {
	NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND(link == nullptr);

	link->set_navigation_layers(p_navigation_layers);
}

uint32_t GodotNavigationServer::link_get_navigation_layers(const RID p_link) const {
	const NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND_V(link == nullptr, 0);

	return link->get_navigation_layers();
}

COMMAND_2(link_set_start_location, RID, p_link, Vector3, p_location) {
	NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND(link == nullptr);

	link->set_start_location(p_location);
}

Vector3 GodotNavigationServer::link_get_start_location(RID p_link) const {
	const NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND_V(link == nullptr, Vector3());

	return link->get_start_location();
}

COMMAND_2(link_set_end_location, RID, p_link, Vector3, p_location) {
	NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND(link == nullptr);

	link->set_end_location(p_location);
}

Vector3 GodotNavigationServer::link_get_end_location(RID p_link) const {
	const NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND_V(link == nullptr, Vector3());

	return link->get_end_location();
}

COMMAND_2(link_set_enter_cost, RID, p_link, real_t, p_enter_cost) {
	NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND(link == nullptr);

	link->set_enter_cost(p_enter_cost);
}

real_t GodotNavigationServer::link_get_enter_cost(const RID p_link) const {
	const NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND_V(link == nullptr, 0);

	return link->get_enter_cost();
}

COMMAND_2(link_set_travel_cost, RID, p_link, real_t, p_travel_cost) {
	NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND(link == nullptr);

	link->set_travel_cost(p_travel_cost);
}

real_t GodotNavigationServer::link_get_travel_cost(const RID p_link) const {
	const NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND_V(link == nullptr, 0);

	return link->get_travel_cost();
}

COMMAND_2(link_set_owner_id, RID, p_link, ObjectID, p_owner_id) {
	NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND(link == nullptr);

	link->set_owner_id(p_owner_id);
}

ObjectID GodotNavigationServer::link_get_owner_id(RID p_link) const {
	const NavLink *link = link_owner.get_or_null(p_link);
	ERR_FAIL_COND_V(link == nullptr, ObjectID());

	return link->get_owner_id();
}

RID GodotNavigationServer::agent_create() const {
	GodotNavigationServer *mut_this = const_cast<GodotNavigationServer *>(this);
	MutexLock lock(mut_this->operations_mutex);
	RID rid = agent_owner.make_rid();
	RvoAgent *agent = agent_owner.get_or_null(rid);
	agent->set_self(rid);
	return rid;
}

COMMAND_2(agent_set_map, RID, p_agent, RID, p_map) {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND(agent == nullptr);

	if (agent->get_map()) {
		if (agent->get_map()->get_self() == p_map) {
			return; // Pointless
		}

		agent->get_map()->remove_agent(agent);
	}

	agent->set_map(nullptr);

	if (p_map.is_valid()) {
		NavMap *map = map_owner.get_or_null(p_map);
		ERR_FAIL_COND(map == nullptr);

		agent->set_map(map);
		map->add_agent(agent);

		if (agent->has_callback()) {
			map->set_agent_as_controlled(agent);
		}
	}
}

COMMAND_2(agent_set_neighbor_distance, RID, p_agent, real_t, p_distance) {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND(agent == nullptr);

	agent->get_agent()->neighborDist_ = p_distance;
}

COMMAND_2(agent_set_max_neighbors, RID, p_agent, int, p_count) {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND(agent == nullptr);

	agent->get_agent()->maxNeighbors_ = p_count;
}

COMMAND_2(agent_set_time_horizon, RID, p_agent, real_t, p_time) {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND(agent == nullptr);

	agent->get_agent()->timeHorizon_ = p_time;
}

COMMAND_2(agent_set_radius, RID, p_agent, real_t, p_radius) {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND(agent == nullptr);

	agent->get_agent()->radius_ = p_radius;
}

COMMAND_2(agent_set_max_speed, RID, p_agent, real_t, p_max_speed) {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND(agent == nullptr);

	agent->get_agent()->maxSpeed_ = p_max_speed;
}

COMMAND_2(agent_set_velocity, RID, p_agent, Vector3, p_velocity) {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND(agent == nullptr);

	agent->get_agent()->velocity_ = RVO::Vector3(p_velocity.x, p_velocity.y, p_velocity.z);
}

COMMAND_2(agent_set_target_velocity, RID, p_agent, Vector3, p_velocity) {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND(agent == nullptr);

	agent->get_agent()->prefVelocity_ = RVO::Vector3(p_velocity.x, p_velocity.y, p_velocity.z);
}

COMMAND_2(agent_set_position, RID, p_agent, Vector3, p_position) {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND(agent == nullptr);

	agent->get_agent()->position_ = RVO::Vector3(p_position.x, p_position.y, p_position.z);
}

COMMAND_2(agent_set_ignore_y, RID, p_agent, bool, p_ignore) {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND(agent == nullptr);

	agent->get_agent()->ignore_y_ = p_ignore;
}

bool GodotNavigationServer::agent_is_map_changed(RID p_agent) const {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND_V(agent == nullptr, false);

	return agent->is_map_changed();
}

COMMAND_4(agent_set_callback, RID, p_agent, ObjectID, p_object_id, StringName, p_method, Variant, p_udata) {
	RvoAgent *agent = agent_owner.get_or_null(p_agent);
	ERR_FAIL_COND(agent == nullptr);

	agent->set_callback(p_object_id, p_method, p_udata);

	if (agent->get_map()) {
		if (p_object_id == ObjectID()) {
			agent->get_map()->remove_agent_as_controlled(agent);
		} else {
			agent->get_map()->set_agent_as_controlled(agent);
		}
	}
}

COMMAND_1(free, RID, p_object) {
	if (map_owner.owns(p_object)) {
		NavMap *map = map_owner.get_or_null(p_object);

		// Removes any assigned region
		LocalVector<NavRegion *> regions = map->get_regions();
		for (uint32_t i = 0; i < regions.size(); i++) {
			map->remove_region(regions[i]);
			regions[i]->set_map(nullptr);
		}

		// Removes any assigned links
		LocalVector<NavLink *> links = map->get_links();
		for (uint32_t i = 0; i < links.size(); i++) {
			map->remove_link(links[i]);
			links[i]->set_map(nullptr);
		}

		// Remove any assigned agent
		LocalVector<RvoAgent *> agents = map->get_agents();
		for (uint32_t i = 0; i < agents.size(); i++) {
			map->remove_agent(agents[i]);
			agents[i]->set_map(nullptr);
		}

		int map_index = active_maps.find(map);
		active_maps.remove_at(map_index);
		active_maps_update_id.remove_at(map_index);
		map_owner.free(p_object);

	} else if (region_owner.owns(p_object)) {
		NavRegion *region = region_owner.get_or_null(p_object);

		// Removes this region from the map if assigned
		if (region->get_map() != nullptr) {
			region->get_map()->remove_region(region);
			region->set_map(nullptr);
		}

		region_owner.free(p_object);

	} else if (link_owner.owns(p_object)) {
		NavLink *link = link_owner.get_or_null(p_object);

		// Removes this link from the map if assigned
		if (link->get_map() != nullptr) {
			link->get_map()->remove_link(link);
			link->set_map(nullptr);
		}

		link_owner.free(p_object);

	} else if (agent_owner.owns(p_object)) {
		RvoAgent *agent = agent_owner.get_or_null(p_object);

		// Removes this agent from the map if assigned
		if (agent->get_map() != nullptr) {
			agent->get_map()->remove_agent(agent);
			agent->set_map(nullptr);
		}

		agent_owner.free(p_object);

	} else {
		ERR_FAIL_COND("Invalid ID.");
	}
}

void GodotNavigationServer::set_active(bool p_active) const {
	GodotNavigationServer *mut_this = const_cast<GodotNavigationServer *>(this);
	MutexLock lock(mut_this->operations_mutex);
	mut_this->active = p_active;
}

void GodotNavigationServer::flush_queries() {
	// In c++ we can't be sure that this is performed in the main thread
	// even with mutable functions.
	MutexLock lock(commands_mutex);
	MutexLock lock2(operations_mutex);
	for (size_t i(0); i < commands.size(); i++) {
		commands[i]->exec(this);
		memdelete(commands[i]);
	}
	commands.clear();
}

void GodotNavigationServer::map_force_update(RID p_map) {
	NavMap *map = map_owner.get_or_null(p_map);
	ERR_FAIL_COND(map == nullptr);

	flush_queries();

	map->sync();
}

void GodotNavigationServer::process(real_t p_delta_time) {
	flush_queries();

	if (!active) {
		return;
	}

	// In c++ we can't be sure that this is performed in the main thread
	// even with mutable functions.
	MutexLock lock(operations_mutex);
	for (uint32_t i(0); i < active_maps.size(); i++) {
		active_maps[i]->sync();
		active_maps[i]->step(p_delta_time);
		active_maps[i]->dispatch_callbacks();

		// Emit a signal if a map changed.
		const uint32_t new_map_update_id = active_maps[i]->get_map_update_id();
		if (new_map_update_id != active_maps_update_id[i]) {
			emit_signal(SNAME("map_changed"), active_maps[i]->get_self());
			active_maps_update_id[i] = new_map_update_id;
		}
	}
}

NavigationUtilities::PathQueryResult GodotNavigationServer::_query_path(const NavigationUtilities::PathQueryParameters &p_parameters) const {
	NavigationUtilities::PathQueryResult r_query_result;

	const NavMap *map = map_owner.get_or_null(p_parameters.map);
	ERR_FAIL_COND_V(map == nullptr, r_query_result);

	// run the pathfinding

	if (p_parameters.pathfinding_algorithm == NavigationUtilities::PathfindingAlgorithm::PATHFINDING_ALGORITHM_ASTAR) {
		// while postprocessing is still part of map.get_path() need to check and route it here for the correct "optimize" post-processing
		if (p_parameters.path_postprocessing == NavigationUtilities::PathPostProcessing::PATH_POSTPROCESSING_CORRIDORFUNNEL) {
			r_query_result.path = map->get_path(p_parameters.start_position, p_parameters.target_position, true, p_parameters.navigation_layers);
		} else if (p_parameters.path_postprocessing == NavigationUtilities::PathPostProcessing::PATH_POSTPROCESSING_EDGECENTERED) {
			r_query_result.path = map->get_path(p_parameters.start_position, p_parameters.target_position, false, p_parameters.navigation_layers);
		}
	} else {
		return r_query_result;
	}

	// add path postprocessing

	// add path stats

	return r_query_result;
}

#undef COMMAND_1
#undef COMMAND_2
#undef COMMAND_4
