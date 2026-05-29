#!/bin/zsh

BASE_URL="http://127.0.0.1:5000/sensors"
SENSOR_ID="sensor_test_$(date +%s)"

# 1. [GET] Initial read (no config file)
# Expects: Default scale 1.0 (200 OK)
echo "--------------------------------------------------------------------"
curl -i -X GET "$BASE_URL/$SENSOR_ID"
echo -e "\n\n"

# 2. [PUT] Try updating before file exists
# Expects: Error because file does not exist (409 Conflict)
echo "--------------------------------------------------------------------"
curl -i -X PUT "$BASE_URL/$SENSOR_ID" \
     -H "Content-Type: application/json" \
     -d '{"scale": 1.5}'
echo -e "\n\n"

# 3. [POST] Create config file (Scale = 2.5)
# Expects: File created successfully (201 Created)
echo "--------------------------------------------------------------------"
curl -i -X POST "$BASE_URL/$SENSOR_ID" \
     -H "Content-Type: application/json" \
     -d '{"scale": 2.5}'
echo -e "\n\n"

# 4. [POST] Try recreating existing file
# Expects: Block duplication and error (409 Conflict)
echo "--------------------------------------------------------------------"
curl -i -X POST "$BASE_URL/$SENSOR_ID" \
     -H "Content-Type: application/json" \
     -d '{"scale": 9.9}'
echo -e "\n\n"

# 5. [GET] Check scale application after creation
# Expects: Measured value equals raw * 2.5 (200 OK)
echo "--------------------------------------------------------------------"
curl -i -X GET "$BASE_URL/$SENSOR_ID"
echo -e "\n\n"

# 6. [PUT] Update existing config (Change scale to 0.5)
# Expects: Overwrite file successfully (200 OK)
echo "--------------------------------------------------------------------"
curl -i -X PUT "$BASE_URL/$SENSOR_ID" \
     -H "Content-Type: application/json" \
     -d '{"scale": 0.5}'
echo -e "\n\n"

# 7. [GET] Final read to verify new scale
# Expects: Measured value reduced by half (200 OK)
echo "--------------------------------------------------------------------"
curl -i -X GET "$BASE_URL/$SENSOR_ID"
echo -e "\n"
