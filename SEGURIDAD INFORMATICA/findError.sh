#!/bin/bash
while true; do
cat input.txt | radamsa | tee last_input.txt | ./insert
test $? -gt 127 && break
done
