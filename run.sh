#!/bin/bash
if [[ ! -e ./txt ]]; then
  echo " Unzip 'texts.zip' into /txt.."
  unzip texts.zip -d txt
fi
./chronicle 

