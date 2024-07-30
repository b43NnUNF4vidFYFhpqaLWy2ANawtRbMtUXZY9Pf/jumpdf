# jumpdf

Jumpdf is a keyboard-focused PDF viewer for documents where one needs to jump between different sections of the document frequently.

## Table of Contents

- [jumpdf](#jumpdf)
  - [Table of Contents](#table-of-contents)
  - [Motivation](#motivation)
  - [Installation](#installation)
    - [Dependencies](#dependencies)
    - [Install](#install)
    - [Uninstall](#uninstall)
  - [Usage](#usage)
    - [Keybindings](#keybindings)
    - [Configuration](#configuration)

## Motivation

When reading a document where one has to switch between various sections, e.g. due to references, it is often necessary to manually scroll back and forth. This is especially inefficient when the sections are far apart.

Jumpdf solves this by having multiple marks/cursors one can switch between. Instead of scrolling, one can create a new mark at the desired location and jump back and forth. In addition, the marks are grouped into groups, which one can also switch between, enabling one to maintain different sets of related marks. For example, in a textbook, one can have a group with marks for the exercises and solutions and another group for reading the material relevant to the exercises. Having these groups also enables one to have a separate window for each group. See the [keybindings](#keybindings) section for more.

Existing PDF viewers like [sioyek](https://sioyek.info/) and [zathura](https://pwmt.org/projects/zathura/) also have marks. However, their marks only record locations, whereas in jumpdf, the current mark follows the current cursor, functioning more like tabs. This allows for maintaining multiple views of the document without having to manually reset the marks.

## Installation

### Dependencies

Required:

- `gtk4` (>= 4.14)
- `poppler-glib` (>= 24.02)
- `sqlite3` (>= 3.45)
- `tomlc99` (>= 1.0.0)

### Install

Build and install using [meson](https://mesonbuild.com/SimpleStart.html):
  
```sh
meson setup release --buildtype=release
meson install -C release
```

### Uninstall

```sh
ninja uninstall -C release
```

## Usage

```sh
# Open PDF file
jumpdf <pdf_file>

# Open multiple PDF files
jumpdf <pdf_file1> <pdf_file2> ...
```

Note that multiple instances can be opened at the same time.

### Keybindings

- <kbd>\<number>\<command></kbd> (repeats \<command> \<number> times)
  - <kbd>j</kbd>, <kbd>k</kbd> (Move down, up)
  - <kbd>h</kbd>, <kbd>l</kbd> (Move left, right. Must not be in center mode)
  - <kbd>d</kbd>, <kbd>u</kbd> (Move down, up half a page)
  - <kbd>+</kbd>, <kbd>-</kbd> (Zoom in, out)
  - <kbd>n</kbd>, <kbd>N</kbd> (Goto next, previous page containing the search string)
- <kbd>0</kbd> (Reset zoom)
- <kbd>c</kbd> (Toggle center mode)
- <kbd>s</kbd> (Fit horizontally to page)
- <kbd>a</kbd> (Fit vertically to page)
- <kbd>gg</kbd>, <kbd>G</kbd>, <kbd>\<number>G</kbd> (Goto first, last page, page \<number>)
- <kbd>f</kbd> (Show link numbers) + <kbd>\<number></kbd> + <kbd>Enter</kbd> (Execute link)
- <kbd>m\<1-9></kbd> (Set current mark to \<1-9>. If the mark hasn't been set, it will be set to the current cursor)
- <kbd>g\<1-9></kbd> (Set curren group to \<1-9>. If the current mark of the group hasn't been set, it will be set to the current cursor)
- <kbd>/</kbd>, <kbd>Esc</kbd> (Show/hide search dialog)
- <kbd>Tab</kbd> (Toggle table of contents)
  - <kbd>j</kbd>, <kbd>k</kbd> (Move down, up)
  - <kbd>/</kbd>, <kbd>Esc</kbd> (Focus/unfocus search entry)
  - <kbd>Enter</kbd> (Goto selected page)

### Configuration

Create config file:

```sh
mkdir -p ~/.config/jumpdf
touch ~/.config/jumpdf/config.toml
```

Default config:

```toml
[Settings]
db_filename = "~/.local/share/jumpdf/jumpdf.db"
steps = 15 # > 0
min_scale = 0.3 # > 0.0
scale_step = 0.1 # > 0.0
```
