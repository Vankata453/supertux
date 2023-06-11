# Convert tiles

## Finish replacing the tiles in data/images/tiles.strf

Have two copies of the game available: One as source code, checked out to the
`tiles-fix` branch, and the other from latest `master` (only binaries are
required).

Launch the second one and navigate to the editor. This will serve as your
reference.

Compile and launch the first one, and open the editor as well. This will help
you identify differences easily.

In the repo of the first copy, open both `data/images/tiles.strf` and
`data/images_old/tiles.strf` side-to-side. The `images` folder is the one that
will remain when the conversion will be complete; the `images_old` folder is
what was there on latest master when the PR forked.

> NOTE: If you downloaded the other copy as source code, you may also replace
> every occurence of "`data/images_old/tiles.strf`" with
> "`data/images/tiles.strf` on the second repo which points to `master`". I only
> added an `images_old` in the PR branch because it allows me to use the split
> view in VSCode.

> NOTE 2: If you use `images_old`, double-check to make sure no tiles were added
> since the PR forked. If any new tiles were added, download an older nightly
> or `git checkout` an older commit. You can re-add those tiles after the PR is
> complete.

In both text editors, scroll down into the "Forest" tilegroup, and spot the line
that contains just `;;` on both files. They serve as a marker for my progress.
This is where the work should be resumed from. (Note that IIRC I skipped a
section or two before the forest, so don't forget to double-check those groups.
Normally, all sections before "Forest" are either 100% done or not at all, but
again, double-check just to be sure.) In the two running games, open the
"Forest" category, and scroll to the corresponding location. This will require
some guesswork as to which IDs correspond to which tiles; try spotting the
patterns of 0's (empty tiles) for help.

Starting from there, find every part where either the visible tiles or the tile
IDs differ, and replace the IDs in the `images` folder to make the editor groups
on `tiles-fix` appear the same was as those on latest `master`. (Basically: copy
visually latest `master` into `tiles-fix`.) Note that the IDs don't necessarily
match, so you can't always copy large blocs over; you have to check each ID
manually. You can use the images files in the `images` folder, as well as the
contents of `data/images/tiles.strf`, to identify which tile has which ID in the
fixed mapping.

> NOTE: **Do not restructure the tilegroups just yet.** Copying the structure
> from latest master is necessary for the steps below. If you'd like to
> restructure the tilegroups to differ from what they are on latest master, do
> that after the PR is complete.

While doing this, make sure to identify, in the old (breaking) version (the
second build), which old (breaking) IDs correspond to each tile. You can use
`data/images_old/tiles.strf` to identify which tile has which old ID. Put the
correspondence between the broken ID and the fixed ID in the file `compat.arr`
following the example below.

Example:

- Before the breaking changes, a certain tilegroup contained the following:
```
(tiles
  1  2  3  4  ;; 4 types of blocks
  5  6  7  8  ;; 4 types of bonus blocks
  9 10 11 12  ;; 4 types of coins
)
```
- After the breaking changes, the same tilegroup contains the following:
```
(tiles
  1  2  3  4  ;; 4 types of blocks (same as above)
 10 11 12 13  ;; 4 types of bonus blocks (same as above but with different IDs,
              ;;                          partly clashing with old IDs)
  7  8 14 15  ;; 4 types of snow tiles (new tiles, clashing with old IDs)
)
```
- In the fixed tiles, the IDs have been remapped to this:
```
;; This is not in a tilegroup; it's just to explain the ID differences
(tiles
  1  2  3  4  ;; 4 types of blocks
  5  6  7  8  ;; 4 types of bonus blocks (reverted to old IDs)
  9 10 11 12  ;; 4 types of coins (with old IDs)
 13 14 15 16  ;; 4 types of snow tiles (added, assigned new IDs to avoid clash)
)
```
- What must be put in the editor tilegroup is the **fixed IDs** corresponding to
  the tiles appearing in the **breaking editor**. That way, no change will be
  visible to the end user between latest master and the tiles fix PR.
```
(tiles
  1  2  3  4  ;; 4 types of blocks (same as usual)
  5  6  7  8  ;; 4 types of bonus blocks (with fixed IDs)
 13 14 15 16  ;; 4 types of snow tiles (with fixed IDs)
)
```
Note that coins do not appear: they have been deprecated, so they should not
show up in the editor. (They are still referenced by the engine, though.)

**While doing this,** take note of every change you make. You should note the
IDs in `data/images_old/tiles.strf` corresponding to which tiles in the new
changes you are bringing in `data/images/tiles.strf`.

To re-use the example above: You would take note of
```
;; That's the second box
  1  2  3  4
 10 11 12 13
  7  8 14 15
```
corresponding to
```
;; That's the fourth box
  1  2  3  4
  5  6  7  8
 13 14 15 16
```
Open `compat.arr` at the root of the repository. Scroll to the end of the file,
just before the comments (`;;`). This is where to put the correspondences. Read
the section below to learn about the format to use.

> NOTE: The comments is a failed project to improve the mappings that I later
> realized couldn't possibly work. You can just delete it.

## `compat.arr` specs

Every line starting with `;;` will be ignored. This will not work if anything
preceedes the `;;`, including whitespace.

Every block must be separated by a `#` standing on its own line. The file must
also start and end with that symbol (comments and whitespace ignored).

Each block can follow one of two formats:
1. Single-ID correspondences, e. g.
```
1 -> 101
2 -> 102
3 -> 103
...
```
2. Block correspondences, e. g.
```
1 2 3 4
5 6 7 8
=>
101 102 103 104
105 106 107 108
```

The latter is provided as convenience for large differing blocks. Both cases
mean that tile ID 1 in the breaking mapping correspond to tile ID 101 in the
fixed ammping, and so on. **Note the difference between -> and =>.**

Also, **don't forget to separate each block with `#` - including the beginning**
**and the end.** See the file itself for an example.

Example, re-using the example above:

```
1  2  3  4
10 11 12 13
7  8  14 15
=>
1  2  3  4
5  6  7  8
13 14 15 16
```

> NOTE: you don't need to put the first row, because the IDs are the same (1,
> 2, 3 and 4 corresponding ot 1, 2, 3 and 4). It doesn't matter if you leave
> them in anyways, because later they'll be stripped away by a script.

```
10 11 12 13
7  8  14 15
=>
5  6  7  8
13 14 15 16
```

## Start here!

From this point, you can start working. You have to convert all tilegroups that
haven't been converted already.

If you encounter tiles in the breaking changes that I forgot to re-add to the
fixed mappings (I believe I've been stopped by something like this):
1. Add the graphics corresponding the the tiles (the image files) at whatever
   place is appropriate.
2. Run the script `tools/gentileids.sh data/images/something/file.png`. It will
   automatically generate new IDs for that file based on the `next-id` in
   `tiles.strf`, and it will update that number accordingly.
3. Look at the bottom of `tiles.strf` to see the new IDs.

When all the tiles have been converted, and any missing tile has been added,
move to the next step.

## Generate the converter data

Super simple: run `tools/setup_convert.sh`. This will automatically create or
update `data/images/convert.txt`. I've already coded something in the game so
that user can convert their levels using that data.

If, at a later point, it turns out a conversion is wrong or missing, it can be
edited/added manually following the format `1234 -> 5678`, alone on a line. The
spaces are important. Windows users be careful, I did not check how the code
reacts to `\r\n`.

After the converter was generated, the PR is ready for review.

## Cleanup the repo

- Delete the `data/images_old` folder - it's no longer needed.
- Delete any extra file that was used throughout the process, like `compat.arr`.
  You can use the GitHub code diff view to help identify the files to delete.
- Optionally, delete the scripts I added in `tools/`. They probably won't be
  useful after the PR is complete.
- Restructure the editor tilegroups as wanted.
- Commit and push.
- **Squash the commits before merging**. The PR duplicated a huge chunk of
  data that will remain in the repository metadata if the commits are not
  squashed.

## Instruct the users on what to do

Here's what the users need to do to convert their recent levels to the fixed
mappings:
1. **Backup your levels.** Do not ignore this step. You've been warned.
2. Open the game with the tiles fix code.
3. Open the editor, and open the level that needs to be converted.
4. Press escape, and select the new option "Convert level" towards the middle.
5. Congrats!
6. Review your levels. Do it! Report any bugs, problems or mismatches to the
   devs.

Note: Do **not** click that option multiple times. Doing so WILL break your
level in irreversible ways! Same thing for using it on levels that don't need
to be converted.

Another note: Keep your backups for a long time. It may take a while before you
notice certain problems in your levels.