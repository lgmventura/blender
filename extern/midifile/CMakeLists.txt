# SPDX-FileCopyrightText: 2024 Blender Authors
#
# SPDX-License-Identifier: GPL-2.0-or-later

set(INC
  include
)

set(INC_SYS
)

set(SRC
  src/Binasc.cpp
  src/MidiEvent.cpp
  src/MidiEventList.cpp
  src/MidiFile.cpp
  src/MidiMessage.cpp
)

set(LIB
)

remove_strict_flags()

blender_add_lib(extern_midifile "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
