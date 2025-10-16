/* *****************************************************************************
 * TIX-100-CXX
 * Copyright (c) 2025 killerbee, Andrea Stacchiotti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * ****************************************************************************/

#include "tis100.h"
#include "sim.hpp"

#include <string>
#include <string_view>

extern "C" {

tis_sim* tis_sim_create() { return new tis_sim(); }

void tis_sim_destroy(tis_sim* sim) { delete sim; }

void tis_sim_add_seed_range(tis_sim* sim, uint32_t begin, uint32_t end) {
	sim->add_seed_range(begin, end);
}

void tis_sim_set_builtin_level_name(tis_sim* sim,
                                    const char* builtin_level_name) {
	sim->set_builtin_level_name(std::string_view(builtin_level_name));
}

#if TIS_ENABLE_LUA
void tis_sim_set_custom_spec_path(tis_sim* sim, const char* custom_spec_path) {
	sim->set_custom_spec_path(std::string(custom_spec_path));
}
void tis_sim_set_custom_spec_code(tis_sim* sim, const char* custom_spec_code,
                                  uint32_t base_seed) {
	sim->set_custom_spec_code(std::string(custom_spec_code), base_seed);
}
#endif

void tis_sim_set_num_threads(tis_sim* sim, uint32_t num_threads) {
	sim->set_num_threads(num_threads);
}

void tis_sim_set_cycles_limit(tis_sim* sim, size_t cycles_limit) {
	sim->set_cycles_limit(cycles_limit);
}

void tis_sim_set_total_cycles_limit(tis_sim* sim, size_t total_cycles_limit) {
	sim->set_total_cycles_limit(total_cycles_limit);
}

void tis_sim_set_cheat_rate(tis_sim* sim, double cheat_rate) {
	sim->set_cheat_rate(cheat_rate);
}

void tis_sim_set_limit_multiplier(tis_sim* sim, double limit_multiplier) {
	sim->set_limit_multiplier(limit_multiplier);
}

void tis_sim_set_T21_size(tis_sim* sim, uint32_t T21_size) {
	sim->set_T21_size(T21_size);
}

void tis_sim_set_T30_size(tis_sim* sim, uint32_t T30_size) {
	sim->set_T30_size(T30_size);
}

void tis_sim_set_run_fixed(tis_sim* sim, bool run_fixed) {
	sim->set_run_fixed(run_fixed);
}

void tis_sim_set_compute_stats(tis_sim* sim, bool compute_stats) {
	sim->set_compute_stats(compute_stats);
}

const struct score* tis_sim_simulate(tis_sim* sim, const char* code) {
	try {
		return &sim->simulate_code(std::string_view(code));
	} catch (const std::exception& e) {
		sim->error_message = e.what();
		return nullptr;
	}
}

const char* tis_sim_get_error_message(const tis_sim* sim) {
	return sim->error_message.c_str();
}

} // extern "C"
