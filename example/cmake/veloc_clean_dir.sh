#!/bin/sh
"$@"
rc=$?
rm -rf scratchdir persdir
exit $rc