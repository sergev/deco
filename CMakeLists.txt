cmake_minimum_required(VERSION 3.15)

project(deco VERSION 3.10 DESCRIPTION "Demos Commander")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Werror -Wshadow)

add_executable(${PROJECT_NAME}
    cap.c
    choice.c
    cmd.c
    com.c
    dir.c
    draw.c
    edit.c
    env.c
    ex.c
    help.c
    hexview.c
    key.c
    main.c
    menu.c
    mh.c
    pw.c
    rec.c
    run.c
    scr.c
    tty.c
    ut.c
    view.c
)
target_link_libraries(${PROJECT_NAME} ncurses)

install(TARGETS ${PROJECT_NAME}
    DESTINATION bin
)
install(FILES deco.1 deco-ru.1
    DESTINATION share/man/man1
)
install(FILES menu profile
    DESTINATION lib/deco
)
install(FILES help/e.general help/e.edit help/e.view help/e.hexview
    DESTINATION lib/deco/help
)
