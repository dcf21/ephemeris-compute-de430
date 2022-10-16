# ephemerisCompute (DE430 version)

`ephemerisCompute` is a command-line tool for producing tables of the positions
of solar system objects over time.

For the Sun, Moon and planets, it extracts positions from the publicly
available NASA DE430 ephemeris (published 2013), which covers the time period
1550 to 2650 AD, typically with an accuracy of a few km. Outside of this time
range, it solves Kepler's equation for the position of an object in an
elliptical orbit, yielding results of much lower accuracy.

For asteroids, it solves Kepler's equation using orbital elements downloaded
from Ted Bowell's `astorb.dat` catalogue.

For comets, it obtains orbital elements from the Minor Planet Center's website.

`ephemerisCompute` was written to produce all of the ephemerides on the website
<https://in-the-sky.org>, which is maintained by the author.

An [older version of this
tool](https://www.github.com/dcf21/ephemeris-compute-de405) is also available,
which uses the NASA DE405 ephemeris (published 1997).

### Supported operating systems

`ephemerisCompute` is written in C and runs in Linux, MacOS, and other
Unix-like operating systems.

### License

This code is distributed under the Gnu General Public License. It is (C)
Dominic Ford 2010 - 2022.

### Set up

Before you start, `ephemerisCompute` needs to download various data from the
internet, including the DE430 ephemeris files, the asteroid catalogue, and the
list of comets.

This can be done with the shell script `setup.sh`. The total download size will
be around 500 MB.

### Docker container

A `Dockerfile` is provided to build `ephemerisCompute`. A `docker compose` script is
provided to build a selection of example starcharts:

```
docker compose build
docker compose run ephmeris-compute-de430
```

This produces a single demo ephemeris. To make other ephemerides, open a shell
within the Docker container as follows:

```
docker run -it ephmeris-compute-de430:v1 /bin/bash
```

### Producing an ephemeris

Running the command-line tool `bin/ephem.bin` will produce a default ephemeris
for Jupiter between 2000 Jan 1 and 2000 Feb 1, at midnight each day:

```asm
dominic@ganymede:~/ephemerisCompute$ ./bin/ephem.bin
2451544.500000000000    3.996320681  2.730993728  1.073274469
2451545.500000000000    3.991757746  2.736868431  1.075903739
2451546.500000000000    3.987185148  2.742736516  1.078530407
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

* `--epoch` [float] - Specify the epoch of the RA/Dec coordinate system, e.g. 2451545.0 for J2000 (default).

* `--objects` [string] - Specify the list of objects to produce ephemerides for. Objects should be separated by commas, e.g. "jupiter, mars" or "P301, A4, 1P/Halley". See below for an explanation of what names are accepted for objects. If multiiple objects are listed, their positions are listed in sets of columns from left to right.

* `--output_binary` [int] - If zero, a text-based ephemeris is produced. If non-zero, then the data is output as a stream of binary data, with type `double`. The first column, the Julian day number, is omitted from binary ephemerides.

* `--output_constellations` [int] - If non-zero, then the final column states the name of the constellation the object is in. Note the fetching this information is one of the slowest routines within ephemerisCompute, so this may have significant performance impact when computing large ephemerides.

* `--use_orbital_elements` [int] - If zero, then the NASA JPL DE430 ephemeris is used to produce the ephemeris. This will give best accuracy (by far). If set to 1, then orbital elements for all objects are used to compute their approximate positions. If set to 2, then algorithms from Jean Meeus's book "Astronomical Algorithms" are used [not currently supported; do not use!]. The positions of comets and asteroids are always computed using orbital elements, since they are not included in DE430.

* `--output_format` [int] - Selects what data should be returned. The following formats are currently supported:

  * -1: XYZ position (ecliptic coordinates at epoch of observation)
  * 0: XYZ position (in ICRS coordinates)
  * 1: RA and Dec (in radians, J2000.0 coordinates; **recommended**)
  * 2: X, Y, Z, RA, Dec, V-band magnitude, phase, angular size
  * 3: As for 2, but also: physical size, albedo, sun_dist, earth_dist, sun_ang_dist, theta_edo, eclLng, eclDist, eclLat

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
* `C<n>`: Comer number `n`. `n` is the line number within the file [Soft00Cmt.txt](http://www.minorplanetcenter.net/iau/Ephemerides/Comets/Soft00Cmt.txt), downloaded from the Minor Planet Center.

## Author

This code was developed by Dominic Ford <https://dcford.org.uk>. It is distributed under the Gnu General Public License V3.

