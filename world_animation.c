/*
 * ASCII World Globe Animation
 *
 * Renders a spinning Earth globe in the terminal using ray-sphere
 * intersection and a Mercator world-map texture.
 *
 * Build:  gcc -O2 -o world_animation world_animation.c -lm
 * Run:    ./world_animation
 * Controls:
 *   Left / Right arrow : adjust horizontal rotation speed
 *   Up   / Down  arrow : tilt globe up / down
 *   + / -             : increase / decrease rotation speed
 *   q                 : quit
 *
 * Copyright (C) 2026 PSD Authors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

/* ── terminal raw-mode helpers ───────────────────────────────────────── */
static struct termios g_orig_termios;

static void restore_terminal(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
    fputs("\033[?25h", stdout);   /* show cursor */
    fputs("\033[0m",   stdout);   /* reset colour */
    fflush(stdout);
}

static void sig_handler(int sig)
{
    (void)sig;
    restore_terminal();
    _exit(0);
}

static void setup_terminal(void)
{
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    atexit(restore_terminal);
    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    struct termios raw = g_orig_termios;
    raw.c_lflag &= (unsigned)~(ECHO | ICANON);
    raw.c_cc[VMIN]  = 0;   /* non-blocking read */
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    /* also make stdin non-blocking at the fd level */
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

/* ── read and apply any pending key-presses ──────────────────────────── */
static void handle_input(double *h_speed, double *v_angle)
{
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 27) {                    /* ESC – start of arrow sequence */
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) break;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) break;
            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': *v_angle -= 5.0; break;  /* Up    */
                    case 'B': *v_angle += 5.0; break;  /* Down  */
                    case 'C': *h_speed += 0.5; break;  /* Right */
                    case 'D': *h_speed -= 0.5; break;  /* Left  */
                }
            }
        } else if (c == '+' || c == '=') {
            *h_speed *= 1.3;
            if (*h_speed > 0.0 && *h_speed < 0.1) *h_speed =  0.1;
            if (*h_speed < 0.0 && *h_speed > -0.1) *h_speed = -0.1;
        } else if (c == '-' || c == '_') {
            *h_speed /= 1.3;
        } else if (c == 'q' || c == 'Q') {
            restore_terminal();
            _exit(0);
        }
        /* clamp vertical tilt to avoid flipping past poles */
        if (*v_angle >  85.0) *v_angle =  85.0;
        if (*v_angle < -85.0) *v_angle = -85.0;
    }
}

/* ── screen / globe geometry ─────────────────────────────────────────── */
#define SW   80          /* screen width  (chars)  */
#define SH   40          /* screen height (chars)  */
#define RADIUS  15.5     /* sphere radius in cell units */
/* character cells are ~2× taller than wide, so squish x by 0.5 */
#define ASPECT  0.5

/* ── world-map texture (72 cols × 36 rows) ───────────────────────────
    NASA-derived land mask (Blue Marble style raster).
    Source raster: https://eoimages.gsfc.nasa.gov/images/imagerecords/57000/57730/land_ocean_ice_2048.png
    Data attribution: NASA Visible Earth
    '#' = land/ice,  '.' = ocean                                       */
static const char *MAP[36] = {
 /*  0 */"########################################################################",
 /*  1 */"###########################################..###########################",
 /*  2 */"####################..##########.......................#################",
 /*  3 */"....####.############....######.....................####################",
 /*  4 */"#..#################.##...###..........#####.###########################",
 /*  5 */"...##############....##...#..........##################################.",
 /*  6 */"...#############....###..............###########################...##...",
 /*  7 */"...############.....####.............###########################........",
 /*  8 */"....##..#########....#...............##########################.........",
 /*  9 */"..........#########.####..............##########################........",
 /* 10 */"...........############.............############################........",
 /* 11 */"...........##########..............#....##.###.###############..........",
 /* 12 */".............#######..............#####....#################............",
 /* 13 */"...............##................###############..#########.............",
 /* 14 */"...............####..............##############...###..###..............",
 /* 15 */"..................##.............#############.....#....##..............",
 /* 16 */"....................#####........#############..........................",
 /* 17 */"....................######............########.........##.##............",
 /* 18 */"....................########..........######............#.##...##.......",
 /* 19 */"....................#########.........######....................##......",
 /* 20 */".....................########.........######..................#.........",
 /* 21 */".....................#######..........######.#..............#####.......",
 /* 22 */"......................#####............####.#.............#######.......",
 /* 23 */"......................###...............#..................##..###......",
 /* 24 */".....................##.................................................",
 /* 25 */".....................##.................................................",
 /* 26 */"........................................................................",
 /* 27 */"......................##.....................#####..##############......",
 /* 28 */".....................###.........#####################################..",
 /* 29 */".........#####.#########.......######################################...",
 /* 30 */"......####################....#######################################...",
 /* 31 */".............................#...................#......................",
 /* 32 */"########################################################################",
 /* 33 */"########################################################################",
 /* 34 */"########################################################################",
 /* 35 */"########################################################################",
};

/* ── shading palette (dark → bright) ─────────────────────────────────── */
static const char LAND_SHADE[]  = ".,+*oO0@#";   /* 9 levels */
static const char OCEAN_SHADE[] = " .:~=+*#@";   /* 9 levels */

enum PixelKind {
    PIX_VOID = 0,
    PIX_LAND = 1,
    PIX_OCEAN = 2,
    PIX_ATMOS = 3,
};

/* ── sample the texture at (lat_deg, lon_deg) ────────────────────────── */
static int is_land(double lat_deg, double lon_deg)
{
    /* clamp longitude to [0,360) */
    while (lon_deg <   0.0) lon_deg += 360.0;
    while (lon_deg >= 360.0) lon_deg -= 360.0;

    int row = (int)((90.0 - lat_deg) / 5.0);
    int col = (int)(lon_deg / 5.0);
    if (row < 0)  row = 0;
    if (row > 35) row = 35;
    if (col < 0)  col = 0;
    if (col > 71) col = 71;
    return MAP[row][col] == '#';
}

/* ── render one frame ────────────────────────────────────────────────── */
static void render_frame(double h_angle, double v_angle,
                         char buf[SH][SW + 1], unsigned char kind[SH][SW])
{
    double v_rad = v_angle * (M_PI / 180.0);  /* vertical tilt in radians */
    /* light direction (fixed in world space, upper-left) */
    static const double lx = -0.6, ly = -0.5, lz = 0.6;
    double llen = sqrt(lx*lx + ly*ly + lz*lz);
    double lxn = lx / llen, lyn = ly / llen, lzn = lz / llen;

    /* half-vector for a small ocean specular glint (viewer is +z) */
    double hx = lxn;
    double hy = lyn;
    double hz = lzn + 1.0;
    double hlen = sqrt(hx*hx + hy*hy + hz*hz);
    hx /= hlen;
    hy /= hlen;
    hz /= hlen;

    /* sphere centre in screen space */
    double cx = SW / 2.0;
    double cy = SH / 2.0;

    for (int y = 0; y < SH; y++) {
        for (int x = 0; x < SW; x++) {
            /* ray origin relative to sphere centre */
            double dx = (x - cx) * ASPECT;
            double dy = (y - cy);

            /* ray-sphere intersection (ray along +z) */
            double r2 = dx * dx + dy * dy;
            double disc = RADIUS * RADIUS - r2;
            if (disc < 0.0) {
                /* thin atmospheric rim outside the sphere */
                double shell = (RADIUS + 1.1) * (RADIUS + 1.1);
                if (r2 <= shell) {
                    double t = (sqrt(r2) - RADIUS) / 1.1; /* 0..1 */
                    int idx = (int)((1.0 - t) * 3.5);
                    if (idx < 0) idx = 0;
                    if (idx > 3) idx = 3;
                    buf[y][x] = " .:-"[idx];
                    kind[y][x] = PIX_ATMOS;
                } else {
                    buf[y][x] = ' ';
                    kind[y][x] = PIX_VOID;
                }
                continue;
            }
            double dz = sqrt(disc);   /* z component of surface normal */

            /* surface normal (unit vector) */
            double nx = dx / RADIUS;
            double ny = dy / RADIUS;
            double nz = dz / RADIUS;

            /* diffuse lighting (Lambert) */
            double ndotl_raw = nx * lxn + ny * lyn + nz * lzn;
            double ndotl = ndotl_raw > 0.0 ? ndotl_raw : 0.0;
            double ndoth = nx * hx + ny * hy + nz * hz;
            if (ndoth < 0.0) ndoth = 0.0;
            double spec = pow(ndoth, 46.0);

            /* smooth day/night split for a softer terminator */
            double daylight = 0.5 + 0.5 * tanh(ndotl_raw * 4.5);

            /* apply vertical tilt (X-axis rotation) */
            double ny_r = ny * cos(v_rad) - nz * sin(v_rad);
            double nz_r = ny * sin(v_rad) + nz * cos(v_rad);

            /* convert normal → lat/lon, then apply horizontal spin */
            double lat = asin(-ny_r) * (180.0 / M_PI);         /* –90…+90 */
            double lon = atan2(nx, nz_r) * (180.0 / M_PI);    /* –180…+180 */
            lon += h_angle;   /* spin the globe */

            int land = is_land(lat, lon);

            double brightness;
            if (land) {
                double day_b = 0.10 + 0.90 * ndotl;
                double night_b = 0.03;

                /* sparse city-lights cue on dark land regions */
                double city = 0.0;
                if (ndotl_raw < 0.0 && fabs(lat) < 60.0) {
                    double n = sin((lat + 91.3) * 12.9898 + lon * 78.233);
                    if (n > 0.985) city = 0.45;
                }
                brightness = night_b * (1.0 - daylight) + day_b * daylight + city;
                kind[y][x] = PIX_LAND;
            } else {
                double day_b = 0.05 + 0.75 * ndotl + 0.50 * spec;
                double night_b = 0.015;
                brightness = night_b * (1.0 - daylight) + day_b * daylight;
                kind[y][x] = PIX_OCEAN;
            }

            if (brightness < 0.0) brightness = 0.0;
            if (brightness > 1.0) brightness = 1.0;

            int shade_idx = (int)(brightness * 8.5);
            if (shade_idx > 8) shade_idx = 8;

            buf[y][x] = land ? LAND_SHADE[shade_idx] : OCEAN_SHADE[shade_idx];
        }
        buf[y][SW] = '\0';
    }
}

/* ── draw a thin border and title ────────────────────────────────────── */
static void print_frame(const char buf[SH][SW + 1],
                        const unsigned char kind[SH][SW],
                        double h_angle, double v_angle, double h_speed)
{
    /* ANSI: move cursor to top-left (no full clear to avoid flicker) */
    fputs("\033[H", stdout);

    /* top border */
    fputs("\033[1;34m", stdout);   /* bold blue */
    fputs("+", stdout);
    for (int i = 0; i < SW; i++) fputc('-', stdout);
    fputs("+\033[K\n", stdout);

    for (int y = 0; y < SH; y++) {
        fputc('|', stdout);
        /* colour by pixel kind from render pass */
        for (int x = 0; x < SW; x++) {
            char c = buf[y][x];
            switch (kind[y][x]) {
                case PIX_LAND:
                    fputs("\033[0;32m", stdout);   /* green land */
                    break;
                case PIX_OCEAN:
                    fputs("\033[0;36m", stdout);   /* cyan ocean */
                    break;
                case PIX_ATMOS:
                    fputs("\033[1;34m", stdout);   /* blue rim */
                    break;
                default:
                    fputs("\033[0m", stdout);      /* black/void */
                    break;
            }
            fputc(c, stdout);
        }
        fputs("\033[1;34m|\033[K\n", stdout);
    }

    /* bottom border + status line */
    fputs("+", stdout);
    for (int i = 0; i < SW; i++) fputc('-', stdout);
    fputs("+\033[K\n", stdout);
    fputs("\033[0;37m", stdout);
    printf("  Lon:%6.1f  Tilt:%+5.1f  Spd:%+5.2f"
           "  [\xe2\x86\x90\xe2\x86\x92]rot [\xe2\x86\x91\xe2\x86\x93]tilt [+-]speed [q]quit\033[K\n",
           fmod(h_angle, 360.0), v_angle, h_speed);
    fputs("\033[0m", stdout);
    fflush(stdout);
}

/* ── CLI help ─────────────────────────────────────────────────────────── */
static void usage(const char *prog)
{
    printf(
        "ASCII World Globe Animation\n"
        "\n"
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Options:\n"
        "  -s SPIN     Initial horizontal rotation in degrees (default: 0.0)\n"
        "  -v SPEED    Rotation speed in degrees/frame      (default: 1.5)\n"
        "              Negative values spin the globe in reverse.\n"
        "  -h, --help  Show this help and exit\n"
        "\n"
        "Examples:\n"
        "  %s -s 180          Start with globe rotated 180 deg\n"
        "  %s -v 3.0          Start spinning at 3 deg/frame\n"
        "  %s -v -2.0         Spin in reverse at 2 deg/frame\n"
        "  %s -s 90 -v -2.0   Start at 90 deg, spinning backwards\n"
        "\n"
        "In-program controls:\n"
        "  Left / Right arrows   Decrease / increase rotation speed by 0.5 deg/frame\n"
        "  Up   / Down  arrows   Tilt globe north / south\n"
        "  +  /  -               Multiply / divide speed by 1.3x\n"
        "  q                     Quit\n"
        "\n"
        "Notes:\n"
        "  Requires an ANSI-colour capable terminal at least 80 columns wide.\n"
        "  Press Ctrl-C or q to exit cleanly.\n",
        prog, prog, prog, prog, prog);
}

/* ── main ─────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    double h_angle = 0.0;   /* initial horizontal rotation (degrees) */
    double h_speed = 1.5;   /* rotation speed (degrees per frame)    */

    /* support --help as well as -h */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        }
    }

    int opt;
    while ((opt = getopt(argc, argv, "s:v:h")) != -1) {
        switch (opt) {
            case 's':
                h_angle = atof(optarg);
                break;
            case 'v':
                h_speed = atof(optarg);
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            default:
                fprintf(stderr, "Try '%s -h' for help.\n", argv[0]);
                return 1;
        }
    }

    setup_terminal();

    /* hide cursor, clear screen once */
    fputs("\033[?25l", stdout);
    fputs("\033[2J",   stdout);

    static char frame[SH][SW + 1];
    static unsigned char kind[SH][SW];
    double v_angle = 0.0;   /* current vertical tilt (degrees) */

    for (;;) {
        handle_input(&h_speed, &v_angle);
        render_frame(h_angle, v_angle, frame, kind);
        print_frame(frame, kind, h_angle, v_angle, h_speed);
        h_angle += h_speed;
        usleep(60000);  /* ~16 fps */
    }

    return 0;
}
