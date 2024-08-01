gnome = import('gnome')

install_data('org.gtk.jumpdf.desktop',
             install_dir : join_paths(get_option('prefix'), 'share', 'applications'))

icon_dir = join_paths(get_option('prefix'), 'share', 'icons', 'hicolor', '512x512', 'apps')
install_emptydir(icon_dir)
# Credit: Icon by amonrat rungreangfangsai (https://www.freepik.com/icon/open-book_2658935)
install_data('jumpdf-icon.png', install_dir : icon_dir)

meson.add_install_script('install_config.sh')

gnome.post_install(gtk_update_icon_cache: true,
                   update_desktop_database: true)