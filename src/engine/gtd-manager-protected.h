/* gtd-manager-protected.h
 *
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "gtd-types.h"

G_BEGIN_DECLS

void                 gtd_manager_load_plugins                    (GtdManager         *manager);

GtdPluginManager*    gtd_manager_get_plugin_manager              (GtdManager         *manager);

void                 _gtd_manager_inject_provider                (GtdManager         *self,
                                                                  GtdProvider        *provider);

G_END_DECLS
