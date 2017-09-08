

# Parsing the ARM v8-A XML instruction database.

This file describes the behavior of `parse_arm_xml`: a tool to extract data
from the ARM v8-A XML instruction database. If you just want to use
`parse_arm_xml`, have a look at the main [README](../../tools/README.md).

## Overview

The tool first reads indexes to determine which instruction files to parse, then
individually processes each of them. The XML data is first translated into an
[`XmlDatabase`](parser.proto) proto to add explicit structure, then gets further
transformed into an [`ArchitectureProto`](../../proto/instructions.proto).

