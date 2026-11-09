# Tarball-O-Matic

A Tracker add-on for Haiku that packs files and folders into `.tar.xz`
archives **without losing large BFS file attributes**, even those larger than 64 KB.

It is a small adaptation of Haiku's Zip-O-Matic: instead of `zip`, it runs
`bsdtar` (libarchive).

## Why

Plain `zip` in Haiku silently drops attributes larger than 64 KB (for example,
pictures stored in attributes). GNU `tar` in Haiku is built without xattr
support. `bsdtar` preserves attributes, but by default it writes every
attribute twice, as `SCHILY` (binary) and `LIBARCHIVE` (base64), so archives
come out much bigger than needed.

Tarball-O-Matic runs:

```
bsdtar --format pax --options pax:xattrheader=SCHILY -J -cvf <archive>.tar.xz <files>
```

and sets the archive's MIME type to `application/x-xz`.

## Requirements

Packages from HaikuDepot: `libarchive` and `libarchive_tools`
(`/bin/bsdtar` must exist).

## Build

You need the `haiku_devel` package. Two ways to build:

**Standalone (no Haiku source tree needed)**

```
cd tarballomatic
make
```

The binary appears in `objects.*/` next to the sources. Copy it to
`~/config/non-packaged/add-ons/Tracker/`.

**Inside the Haiku source tree**

1. Copy the `tarballomatic` directory to `src/add-ons/tracker/` of the tree.
2. Add this line to `src/add-ons/tracker/Jamfile`, next to the line for
   `zipomatic`:

   ```
   SubInclude HAIKU_TOP src add-ons tracker tarballomatic ;
   ```

3. From the root of the tree run `jam -q TarballOMatic`.
4. Copy the resulting binary (look under
   `generated/objects/haiku/<arch>/release/add-ons/tracker/tarballomatic/`)
   to `~/config/non-packaged/add-ons/Tracker/`.

The default keyboard shortcut is Opt+Alt+O. It is defined in
`tarballomatic/TarballOMatic.rdef` (`BEOS:default_shortcut`); change it there
if it clashes with something else.

## Unpacking with Expander

Expander calls GNU `tar`, which silently loses attributes. Put the rules from
`expander-rules/all` into
`~/config/non-packaged/data/expander/rules/all`, so that Expander uses
`bsdtar` instead.

To check that attributes survived, run `listattr -l <file or folder>` before
packing and after unpacking.

## Changes relative to Zip-O-Matic

- `bsdtar` is started instead of `zip` (and its output is read from stderr);
- the process-name checks for pause, resume and stop look for `bsdtar`;
- the archive extension is `.tar.xz` and its MIME type is `application/x-xz`;
- the processed-files counter is no longer off by one;
- the application and its signature were renamed
  (`application/x-vnd.haiku.tarball-o-matic`).

## License

MIT, same as the original code. See `LICENSE`.
Zip-O-Matic is by Jonas Sundström and Peter Folk, Haiku, Inc.
