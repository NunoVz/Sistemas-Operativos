#!/bin/bash

# Clear all shared memory segments
ipcs -m | awk 'NR>3 {print $2}' | xargs -r ipcrm shm

# Clear all semaphores
ipcs -s | awk 'NR>3 {print $2}' | xargs -r ipcrm sem
