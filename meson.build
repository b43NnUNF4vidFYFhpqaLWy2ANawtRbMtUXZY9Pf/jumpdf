project('jumpdf', 'c',
        version : '0.1',
        license : 'MIT',
        meson_version : '>=1.4.0',
        default_options : ['c_std=c99'])

app_name = 'jumpdf'
app_id = 'com.github.b43NnUNF4vidFYFhpqaLWy2ANawtRbMtUXZY9Pf.jumpdf'

conf_data = configuration_data()
conf_data.set('app_name', app_name)
conf_data.set('app_id', app_id)
configure_file(input : 'src/project_config.h.in',
               output : 'project_config.h',
               configuration : conf_data)

src_files = [
        'src/main.c',
        'src/config.c',
        'src/file_utils.c',
        'src/app.c',
        'src/win.c',
        'src/statusline.c',
        'src/database.c',
        'src/viewer.c',
        'src/render.c',
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

executable(app_name,
           sources : src_files,
           dependencies : deps,
           install : true)

subdir('data')