#!/bin/zsh

# Base URL for the local Flask server
BASE_URL="http://127.0.0.1:5000/files"

echo "=== Creating a file with a specific name ==="
curl -X POST "$BASE_URL" \
     -H "Content-Type: application/json" \
     -d '{"name": "lab_notes.txt", "content": "Initial content for lab 11."}'
echo -e "\n"

echo "=== Creating a file with content only (auto-generated name) ==="
curl -X POST "$BASE_URL" \
     -H "Content-Type: application/json" \
     -d '{"content": "Randomly named file content."}'
echo -e "\n"

echo "=== Listing all files in the directory ==="
curl -X GET "$BASE_URL"
echo -e "\n"

echo "=== Reading the content of the named file ==="
curl -X GET "$BASE_URL/lab_notes.txt"
echo -e "\n"

echo "=== Updating the content of the named file ==="
curl -X PUT "$BASE_URL/lab_notes.txt" \
     -H "Content-Type: application/json" \
     -d '{"content": "This content has been completely updated."}'
echo -e "\n"

echo "=== Reading the file again to verify the update ==="
curl -X GET "$BASE_URL/lab_notes.txt"
echo -e "\n"

echo "=== Deleting the named file ==="
curl -X DELETE "$BASE_URL/lab_notes.txt"
echo -e "\n"

echo "=== Final list to check if it was successfully removed ==="
curl -X GET "$BASE_URL"
echo -e "\n"
