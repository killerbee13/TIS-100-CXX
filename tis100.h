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

#ifndef TIS100_H
#define TIS100_H

// Public FFI API

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/// Result of a full sim run
struct score {
	size_t cycles;
	size_t nodes;
	size_t instructions;
	unsigned int random_test_ran;
	unsigned int random_test_valid;
	bool validated;
	bool achievement;
	bool cheat;
	bool hardcoded;
};

/// Opaque tis_sim struct
struct tis_sim;

// Constructor and destructor

/// Returns a pointer to a new tis_sim instance
struct tis_sim* tis_sim_create(void);
/// Frees a tis_sim instance
void tis_sim_destroy(struct tis_sim* sim);

// Set simulation parameters
void tis_sim_add_seed_range(struct tis_sim* sim, uint32_t begin, uint32_t end);
void tis_sim_set_builtin_level_name(struct tis_sim* sim,
                                    const char* builtin_level_name);
#if TIS_ENABLE_LUA
void tis_sim_set_custom_spec_path(struct tis_sim* sim,
                                  const char* custom_spec_path);
void tis_sim_set_custom_spec_code(struct tis_sim* sim,
                                  const char* custom_spec_code,
                                  uint32_t base_seed);
#endif
void tis_sim_set_num_threads(struct tis_sim* sim, uint32_t num_threads);
void tis_sim_set_cycles_limit(struct tis_sim* sim, size_t cycles_limit);
void tis_sim_set_total_cycles_limit(struct tis_sim* sim,
                                    size_t total_cycles_limit);
void tis_sim_set_cheat_rate(struct tis_sim* sim, double cheat_rate);
void tis_sim_set_limit_multiplier(struct tis_sim* sim, double limit_multiplier);
void tis_sim_set_T21_size(struct tis_sim* sim, uint32_t T21_size);
void tis_sim_set_T30_size(struct tis_sim* sim, uint32_t T30_size);
void tis_sim_set_run_fixed(struct tis_sim* sim, bool run_fixed);
void tis_sim_set_compute_stats(struct tis_sim* sim, bool compute_stats);

// get simulation results
const char* tis_sim_get_error_message(const struct tis_sim* sim);

/// Run the simulation
const struct score* tis_sim_simulate(struct tis_sim* sim, const char* code);

#ifdef __cplusplus
}
#endif

#endif // TIS100_H
