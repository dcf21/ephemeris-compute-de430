# ephemerisCompute (DE4xx version)

`ephemerisCompute` is a command-line tool for producing tables of the positions
of solar system objects over time.

For the Sun, Moon and planets, it extracts positions from the publicly
available NASA JPL DE4xx series of ephemerides (DE405, DE430, DE431, DE440 and
DE441 are all supported). These cover various time periods from AD 1000 to AD
16000, typically with an accuracy of a few km. Outside of this time range, this
tool solves Kepler's equation for the position of each object assuming an
elliptical orbit, yielding results of much lower accuracy.

For asteroids, it solves Kepler's equation using orbital elements downloaded
from the [Asteroid Orbital Elements
Database](https://asteroid.lowell.edu/astorb/) (`astorb.dat`) hosted by the
Lowell Observatory.

For comets, it obtains orbital elements from the [Minor Planet
Center](https://www.minorplanetcenter.net/data)'s website.

`ephemerisCompute` was written to produce all the ephemerides on the website
[https://in-the-sky.org](https://in-the-sky.org), which is maintained by the
author.

An [older version of this
tool](https://www.github.com/dcf21/ephemeris-compute-de405) is also available,
which exclusively uses the NASA DE405 ephemeris (published 1997).

### Supported operating systems

`ephemerisCompute` is written in C and runs in Linux, MacOS, and most other
Unix-like operating systems.

### License

This code is distributed under the Gnu General Public License. It is © Dominic
Ford 2010 - 2026.

### Set up

Before you start, `ephemerisCompute` needs to download various data from the
internet, including the DE4xx ephemeris files, the asteroid catalogue, and the
list of comets.

This can be done with the shell script `setup.sh`. The total download size will
be around 500 MB.

Once you have built `ephemerisCompute`, you must not change its location within
your file system.  During the build process, the absolute path to the
downloaded data files is stored, and the code will be unable to find these data
files if their path changes. If you move the code, you must fully rebuild it:

```
make clean
./setup.sh
```

### Docker container

A `Dockerfile` is provided to build `ephemerisCompute`. A `docker-compose`
script is provided to build the software and display a single demo ephemeris:

```
docker compose build
docker compose run ephemeris-compute-de4xx
```

To make other ephemerides, open a shell within the Docker container as follows:

```
docker run -it ephemeris-compute-de4xx:v8 /bin/bash
```

### Producing an ephemeris

Running the command-line tool `./bin/ephem.bin` will produce a default
ephemeris for Jupiter between 2000 Jan 1 and 2000 Feb 1, at midnight each day:

```
dominic@ganymede:~/ephemerisCompute$ ./bin/ephem.bin
2451544.500000000000   3.9963890377035507 2.7309425445995013 1.0732394487750072
2451545.500000000000   3.9918297581095996 2.7368109182255269 1.0758659226594827
2451546.500000000000   3.9872608492979653 2.7426726205786123 1.0784897750709959
...
```

The first time you run the tool, it needs to convert the ASCII data files you
downloaded into a binary format, which will typically take a few seconds before
any output is produced. The binary data is cached, leading to near
instantaneous performance subsequently.

In this output, the columns are Julian day number, and the XYZ position of
Jupiter, measured in AU, relative to the centre of mass of the solar system.

The following command-line arguments can be used to customise the ephemeris:

* `--jd_min` [float] - Specify the Julian day number at which the ephemeris should begin.

* `--jd_max` [float] - Specify the Julian day number at which the ephemeris should end.

* `--jd_step` [float] - Specify the interval between the lines in the ephemeris, in days.

* `--jd_list` [string] - Specify an explicit list of Julian day numbers to calculate (optional). If this is specified, this list overrides <jd_min>, <jd_max> and <jd_step>.

* `--latitude` [float] - The latitude of the observation site (deg); only used if topocentric correction enabled.

* `--longitude` [float] - The longitude of the observation site (deg); only used if topocentric correction enabled.

* `--enable_topocentric_correction` [int] - Set to either 0 (return geocentric coordinates) or 1 (return topocentric coordinates).

* `--epoch` [float] - Specify the epoch of the RA/Dec coordinate system, e.g. 2451545.0 for J2000 (default).

* `--objects` [string] - Specify the list of objects to produce ephemerides for. Objects should be separated by commas, e.g. "jupiter, mars" or "P301, A4, 1P/Halley". See below for an explanation of what names are accepted for objects. If multiiple objects are listed, their positions are listed in sets of columns from left to right.

* `--output_binary` [int] - If zero, a text-based ephemeris is produced. If non-zero, then the data is output as a stream of binary data, with type `double`. The first column, the Julian day number, is omitted from binary ephemerides.

* `--output_constellations` [int] - If non-zero, then the final column states the name of the constellation the object is in. Note the fetching this information is one of the slowest routines within ephemerisCompute, so this may have significant performance impact when computing large ephemerides. If binary output is requested, then the constellation name is output as an abbreviated name, with a fixed width of 8 bytes, at the end of each record.

* `--use_orbital_elements` [int] - If zero, then the NASA JPL DE4xx ephemeris is used to produce the ephemeris. This will give best accuracy (by far). If set to 1, then orbital elements for all objects are used to compute their approximate positions. If set to 2, then algorithms from Jean Meeus's book "Astronomical Algorithms" are used [not currently supported; do not use!]. The positions of comets and asteroids are always computed using orbital elements, since they are not included in DE4xx.

* `--output_format` [int] - Selects what data should be returned. The following formats are currently supported:

  * Binary mode:
    * -1: X, Y, Z position (ecliptic coordinates at epoch of observation) [3 columns]
    * 0: X, Y, Z position (in ICRS coordinates) [3 columns]
    * 1: RA, Dec (in radians, J2000.0 coordinates; **recommended**) [2 columns]
    * 2: X, Y, Z, RA, Dec, V-band magnitude, phase, angular size [8 columns]
    * 3: As for 2, but also: physical size, albedo, sun_dist, earth_dist, sun_ang_dist, theta_edo, eclLng, eclDist, eclLat [17 columns]
  * Text mode:
    * -1: JD, X, Y, Z position (ecliptic coordinates at epoch of observation) [4 columns]
    * 0: JD, X, Y, Z position (in ICRS coordinates) [4 columns]
    * 1: JD, RA, Dec (in radians, J2000.0 coordinates; **recommended**) [3 columns]
    * 2: JD, X, Y, Z, RA, Dec, V-band magnitude, phase, angular size [9 columns]
    * 3: As for 2, but also: physical size, albedo, sun_dist, earth_dist, sun_ang_dist, theta_edo, eclLng, eclDist, eclLat [18 columns]

### Object names

This section lists the names which are recognised by the `--objects` command-line argument:

* `p1`, `pmercury`, `mercury`: Mercury
* `p2`, `pvenus`, `venus`: Venus
* `p3`, `pearth`, `earth`: Earth
* `p301`, `pmoon`, `moon`: The Moon
* `p4`, `pmars`, `mars`: Mars
* `p5`, `pjupiter`, `jupiter`: Jupiter
* `p6`, `psaturn`, `saturn`: Saturn
* `p7`, `puranus`, `uranus`: Uranus
* `p8`, `pneptune`, `neptune`: Neptune
* `p9`, `ppluto`, `pluto`: Pluto
* `A<n>`: Asteroid number `n`, e.g. `A1` for Ceres, or `A4` for Vesta
* `C/1995 O1`. Comets may be referred to by their names in this format
* `1P/Halley`. Comets may be referred to by their names in this format
* `0001P`. Periodic comets may be referred to by their names in the format %4dP
* `CJ95O010`. Comets may be referred to by their Minor Planet Center designations
* `C<n>`: Comer number `n`. `n` is the line number within the file [Soft00Cmt.txt](http://www.minorplanetcenter.net/iau/Ephemerides/Comets/Soft00Cmt.txt), downloaded from the Minor Planet Center (MPC).

## Ephemeris files

When the software is installed, data files containing the orbital elements of asteroids and comets are downloaded from
the following sources:

* Asteroids, from the [Lowell Observatory](https://asteroid.lowell.edu/astorb/) website.
* Comets, from the [Minor Planet Center](https://www.minorplanetcenter.net/data) (MPC) website

The orbital elements published on these websites are typically accurate for a few years either side of the epoch when
they were downloaded, but will give erroneous positions outside this time range due to orbital perturbations. The exact
timescale for orbital perturbation is unique to each object, depending on its proximity to sources of perturbation -
in particular, Jupiter.

If a high degree of accuracy is required over a longer time span, the software supports the use of multiple orbital
element files downloaded at different epochs. These should be placed in a directory `~/astorb_archive/` in the user's
home directory; they should match the following wildcards:

* `~/astorb_archive/astorb_*.dat`
* `~/astorb_archive/Soft00Cmt_*.dat*`

The software automatically reads the epoch specified within each data file and for each query will choose the two data
files that are closest before and after the requested epoch. The predicted positions from the two data files are
linearly interpolated to ensure that the output ephemerides are always continuous and differentiable.

### Change history

**Version 8.0** (18 Apr 2026) - Adds support for DE405, DE430, DE431, DE440 and DE441.

**Version 7.0** (10 Nov 2025) - Support using multiple files of asteroid / comet orbital elements at different epochs. Add socket-based server/client interface for rapid queries.

**Version 6.0** (23 Feb 2025) - Fix download links and improve documentation.

**Version 5.0** (7 Jan 2025) - Added `--jd_list` command-line option.

**Version 4.0** (23 Sept 2024) - Added optional topocentric correction.

**Version 3.0** (23 Aug 2024) - Added corrections for light travel time and annual aberration. Improvements to numerical stability when calculating hyperbolic orbits.

**Version 2.0** (16 Oct 2022) - Initial public release.

## Author

This code was developed by Dominic Ford
[https://dcford.org.uk](https://dcford.org.uk). It is distributed under the Gnu
General Public License V3.

