##############################
# External Project description
set(mvnRepo https://repo1.maven.org/maven2)
set(groupId com/esotericsoftware/kryo)
set(artifactId kryo)
set(ver 2.24.0)

set(dUrl ${mvnRepo}/${groupId}/${artifactId}/${ver}/${artifactId}-${ver}.jar)
set(tDir ${CMAKE_CURRENT_BINARY_DIR}/${artifactId}-${ver}.jar)
set(logStr "Downloading ${dUrl}")

file(DOWNLOAD ${dUrl}.md5 ${tDir}.md5)
file(STRINGS ${tDir}.md5 MD5_HASH)
message(STATUS ${logStr})
file(DOWNLOAD ${dUrl} ${tDir} EXPECTED_HASH MD5=${MD5_HASH})
execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${tDir} --format=zip com/*
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/cp)
