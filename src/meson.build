sources = [
    'main.c',
    'config.c',
    'toml.c',
    'utils.c',
    'app.c',
    'window.c',
    'statusline.c',
    'database.c',
    'viewer.c',
    'renderer.c',
    'input_cmd.c',
    'input_FSM.c',
    'page.c',
    'viewer_info.c',
    'viewer_cursor.c',
    'viewer_search.c',
    'viewer_links.c',
    'viewer_mark_group.c',
    'viewer_mark_manager.c',
]

conf_data = configuration_data()
conf_data.set('app_name', app_name)
conf_data.set('app_id', app_id)
configure_file(input : 'project_config.h.in',
           output : 'project_config.h',
           configuration : conf_data)

executable(app_name,
       sources : sources,
       dependencies : deps,
       install : true)