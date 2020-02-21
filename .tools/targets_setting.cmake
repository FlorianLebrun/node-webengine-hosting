

macro(set_project_common_output target outputDir)
  set_target_properties(${target} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${outputDir}
    LIBRARY_OUTPUT_DIRECTORY ${outputDir}
    RUNTIME_OUTPUT_DIRECTORY ${outputDir}
    COMPILE_PDB_OUTPUT_DIRECTORY ${outputDir}
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${outputDir}
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${outputDir}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${outputDir}
    COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG ${outputDir}
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${outputDir}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${outputDir}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${outputDir}
    COMPILE_PDB_OUTPUT_DIRECTORY_RELEASE ${outputDir}
  )
endmacro()


function(append_group_sources sources)
  cmake_parse_arguments(ARG "" "FILTER" "DIRECTORIES;EXCLUDES" ${ARGN})

  if(ARG_EXCLUDES)
    foreach(L_cexclude ${ARG_EXCLUDES})
      get_filename_component(L_cexclude ${L_cexclude} ABSOLUTE)
      list(APPEND L_EXCLUDES_ABS ${L_cexclude})
    endforeach()
  endif()

  string(REGEX MATCHALL "[^\|]+" subfilters ${ARG_FILTER})
  foreach(cname ${ARG_DIRECTORIES})

    get_filename_component(cdir "${CMAKE_CURRENT_SOURCE_DIR}/${cname}" ABSOLUTE)
    
    set(cgroup ${cname})
    string(REGEX REPLACE "\\.\\./" "" cgroup ${cname})
    string(REGEX REPLACE "\\./" "Sources/" cgroup ${cgroup})
    string(REGEX REPLACE "/" "\\\\" cgroup ${cgroup})
      
    foreach(cfilter ${subfilters})
      #message(STATUS "glob: ${cgroup} | ${cname} ${cfilter} | ${cdir}")
      file(GLOB founds "${cdir}/${cfilter}")
      list(REMOVE_ITEM founds "" ${L_EXCLUDES_ABS})
      list(APPEND founds_files ${founds})
      source_group(${cgroup} FILES ${founds})
    endforeach()
  endforeach()
  set(${sources} ${founds_files} PARENT_SCOPE)
endfunction()
