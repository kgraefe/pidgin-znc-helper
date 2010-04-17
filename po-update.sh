#!/bin/bash
cd po
echo "Updating POT template..."
intltool-update -po
echo "Checking german language file..."
intltool-update de
