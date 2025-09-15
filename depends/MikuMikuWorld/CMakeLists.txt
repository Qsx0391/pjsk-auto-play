add_library(MikuMikuWorld STATIC
    BinaryReader.cpp
    BinaryWriter.cpp
    MmwFile.cpp
    MmwIO.cpp
    Note.cpp
    Score.cpp
    ScoreConverter.cpp
    SusParser.cpp
    Tempo.cpp
)

target_include_directories(MikuMikuWorld PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)