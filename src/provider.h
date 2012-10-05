/**
 * Copyright (C) 2012 Richard Burnison
 *
 * This file is part of Clip.
 *
 * Clip is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Clip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Clip.  If not, see <http://www.gnu.org/licenses/>.
 */

typedef struct provider ClipboardProvider;

ClipboardProvider* clip_provider_new(void);
void clip_provider_free(ClipboardProvider* provider);

char* clip_provider_get_current(ClipboardProvider* provider);
void clip_provider_free_current(char* current);

void clip_provider_set_current(ClipboardProvider* provider, char* text);
void clip_provider_clear(ClipboardProvider* provider);

