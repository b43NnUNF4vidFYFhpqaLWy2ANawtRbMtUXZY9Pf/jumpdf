project('jumpdf', 'c',
        version : '0.1',
        default_options : ['c_std=c99'])

src_files = [
        'src/main.c',
        'src/app.c',
        'src/win.c',
        'src/viewer.c',
        'src/input_FSM.c',
        'src/viewer_info.c',
        'src/viewer_cursor.c',
        'src/viewer_search.c',
        'src/viewer_links.c',
        ]

cc = meson.get_compiler('c')

gtk_dep = dependency('gtk4')
poppler_glib_dep = dependency('poppler-glib')
math_dep = cc.find_library('m', required : true)

executable('jumpdf',
           sources : src_files,
           dependencies : [gtk_dep, poppler_glib_dep, math_dep],
           install : true)