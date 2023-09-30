
find_package(Java REQUIRED)
include(UseJava)

#
# Java: jagent.jar
#
add_jar(
  jagent
  SOURCES jagent.java
  MANIFEST manifest.txt
  INCLUDE_JARS ${CP_JAR}
)
add_dependencies(jagent cloud_profiler)

ADD_CUSTOM_COMMAND(TARGET jagent POST_BUILD
  COMMAND "${Java_JAVAC_EXECUTABLE}" -cp ${CP_JAR} ${CMAKE_CURRENT_SOURCE_DIR}/jagent.java -d .
  COMMAND "cp" -pr ${CP_LIBDIR}/cp .
  COMMAND "${Java_JAR_EXECUTABLE}" cfm jagent.jar
                                   ${CMAKE_CURRENT_SOURCE_DIR}/manifest.txt
                                   *.class
                                   cp/swig/*.class 
                                   cp/java_agent/*.class 
)
