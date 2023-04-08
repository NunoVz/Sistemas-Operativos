#!/bin/bash

# Clear all shared memory segments
ipcs -m | awk '{print $2}' | grep -v "ID" | xargs -r ipcrm shm

# Clear all semaphores
ipcs -s | awk '{print $2}' | grep -v "ID" | xargs -r ipcrm sem
