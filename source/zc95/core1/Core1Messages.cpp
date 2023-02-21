/*
 * ZC95
 * Copyright (C) 2021  CrashOverride85
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "Core1Messages.h"


#include "Core1.h"

mutex_t g_collar_message_mutex;

mutex_t g_core1_suspend_mutex;
struct semaphore g_core1_suspend_sem;

CCollarComms::collar_message g_collar_message;

void messages_init()
{
    mutex_init(&g_collar_message_mutex);
    mutex_init(&g_core1_suspend_mutex );
    sem_init(&g_core1_suspend_sem, 1, 1);
}
