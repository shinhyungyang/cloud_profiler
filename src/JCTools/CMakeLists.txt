##############################
# External Project description
set(mvnRepo https://repo1.maven.org/maven2)
set(groupId org/jctools)
set(artifactId jctools-core)
set(ver 3.1.0)

set(dUrl ${mvnRepo}/${groupId}/${artifactId}/${ver}/${artifactId}-${ver}.jar)
set(tDir ${CMAKE_CURRENT_BINARY_DIR}/${artifactId}-${ver}.jar)
set(logStr "Downloading ${dUrl}")

file(DOWNLOAD ${dUrl}.md5 ${tDir}.md5)
file(STRINGS ${tDir}.md5 MD5_HASH)
message(STATUS ${logStr})
file(DOWNLOAD ${dUrl} ${tDir} EXPECTED_HASH MD5=${MD5_HASH})
execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${tDir} --format=zip org/*
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/cp)
