project('jumpdf', 'c',
        version : '0.1',
        license : 'MIT',
        meson_version : '>=1.4.0',
        default_options : ['c_std=c99'])

src_files = [
        'src/main.c',
        'src/config.c',
        'src/file_utils.c',
        'src/app.c',
        'src/win.c',
        'src/statusline.c',
        'src/database.c',
        'src/viewer.c',
        'src/input_FSM.c',
        'src/viewer_info.c',
        'src/viewer_cursor.c',
        'src/viewer_search.c',
        'src/viewer_links.c',
        'src/viewer_mark_group.c',
        'src/viewer_mark_manager.c',
        ]

cc = meson.get_compiler('c')

deps = [
        dependency('gtk4', required : true, version : '>=4.14'),
        dependency('poppler-glib', required : true, version : '>=24.02'),
        dependency('sqlite3', required : true, version : '>=3.45'),
        cc.find_library('toml', required : true),
        cc.find_library('m', required : true),
]

executable('jumpdf',
           sources : src_files,
           dependencies : deps,
           install : true)

subdir('data')