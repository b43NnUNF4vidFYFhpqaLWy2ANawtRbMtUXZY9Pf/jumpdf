project('jumpdf', 'c',
        version : '0.1',
        default_options : ['c_std=c99'])

src_files = [
        'src/main.c',
        'src/app.c',
        'src/win.c',
        'src/viewer.c',
        'src/input_FSM.c',
        ]

gtk_dep = dependency('gtk4')
poppler_glib_dep = dependency('poppler-glib')

executable('jumpdf',
           sources : src_files,
           dependencies : [gtk_dep, poppler_glib_dep],
           install : true)