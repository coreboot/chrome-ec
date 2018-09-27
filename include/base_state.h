/* Copyright 2018 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/**
 * Call board specific base_force_state function.
 * Force the current state of the base, with 0 meaning detached,
 * 1 meaning attached and 2 meaning reset to the original state.
 */
void base_force_state(int state);
