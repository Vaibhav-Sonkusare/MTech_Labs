#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    double x, y;
} Point;

typedef struct {
    Point p1, p2;
    int id;
} Segment;

typedef struct {
    double x;
    int segmentIndex;
    int type;
} Event;

int compareEvents(const void *a, const void *b) {
    Event *e1 = (Event *)a;
    Event *e2 = (Event *)b;
    if (e1->x == e2->x)
        return e1->type - e2->type;
    return (e1->x < e2->x) ? -1 : 1;
}

int orientation(Point a, Point b, Point c) {
    double val = (b.y - a.y) * (c.x - b.x) - (b.x - a.x) * (c.y - b.y);
    if (val == 0) return 0;
    return (val > 0) ? 1 : 2;
}

int onSegment(Point p, Point q, Point r) {
    return (q.x <= fmax(p.x, r.x) && q.x >= fmin(p.x, r.x) &&
            q.y <= fmax(p.y, r.y) && q.y >= fmin(p.y, r.y));
}

int doIntersect(Segment s1, Segment s2) {
    Point p1 = s1.p1, q1 = s1.p2;
    Point p2 = s2.p1, q2 = s2.p2;

    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);

    if (o1 != o2 && o3 != o4)
        return 1;

    if (o1 == 0 && onSegment(p1, p2, q1)) return 1;
    if (o2 == 0 && onSegment(p1, q2, q1)) return 1;
    if (o3 == 0 && onSegment(p2, p1, q2)) return 1;
    if (o4 == 0 && onSegment(p2, q1, q2)) return 1;

    return 0;
}

Point intersectionPoint(Segment s1, Segment s2) {
    Point p1 = s1.p1, p2 = s1.p2;
    Point p3 = s2.p1, p4 = s2.p2;
    Point inter;

    double A1 = p2.y - p1.y;
    double B1 = p1.x - p2.x;
    double C1 = A1 * p1.x + B1 * p1.y;

    double A2 = p4.y - p3.y;
    double B2 = p3.x - p4.x;
    double C2 = A2 * p3.x + B2 * p3.y;

    double determinant = A1 * B2 - A2 * B1;

    if (determinant == 0) {
        inter.x = inter.y = 0;
    } else {
        inter.x = (B2 * C1 - B1 * C2) / determinant;
        inter.y = (A1 * C2 - A2 * C1) / determinant;
    }

    return inter;
}

Segment *create_segment_list(int *n) {
    int points[][4] = {{0, 0, 4, 4},
                       {0, 4, 4, 0},
                       {0, 6, 4, 0}};

    *n = sizeof(points) / sizeof(points[0]);

    Segment *s_list = calloc(*n, sizeof(Segment));

    for (int i=0; i<*n; i++) {
        s_list[i].id = i;
        s_list[i].p1.x = points[i][0];
        s_list[i].p1.y = points[i][1];
        s_list[i].p2.x = points[i][2];
        s_list[i].p2.y = points[i][3];
    }

    return s_list;
}

int main() {
    int n;

    Segment *segments = create_segment_list(&n);

    if (n <= 0) {
        printf("Number of segments must be positive.\n");
        return 1;
    }
    printf("%d", n);

    Event events[2 * n];

    for (int i = 0; i < n; i++) {

        if (segments[i].p1.x > segments[i].p2.x) {
            Point temp = segments[i].p1;
            segments[i].p1 = segments[i].p2;
            segments[i].p2 = temp;
        }

        events[2 * i].x = segments[i].p1.x;
        events[2 * i].segmentIndex = i;
        events[2 * i].type = 0;

        events[2 * i + 1].x = segments[i].p2.x;
        events[2 * i + 1].segmentIndex = i;
        events[2 * i + 1].type = 1;
    }

    qsort(events, 2 * n, sizeof(Event), compareEvents);

    int active[n];
    int activeCount = 0;
    int found = 0;

    printf("\nIntersections:\n");

    for (int i = 0; i < 2 * n; i++) {
        Event e = events[i];
        int idx = e.segmentIndex;

        if (e.type == 0) {
            for (int j = 0; j < activeCount; j++) {
                if (doIntersect(segments[idx], segments[active[j]])) {
                    Point inter = intersectionPoint(segments[idx], segments[active[j]]);
                    printf("Segment %d intersects with Segment %d at (%.2lf, %.2lf)\n",
                           segments[idx].id + 1, segments[active[j]].id + 1,
                           inter.x, inter.y);
                    found = 1;
                }
            }
            active[activeCount++] = idx;
        } else {
            for (int j = 0; j < activeCount; j++) {
                if (active[j] == idx) {
                    active[j] = active[--activeCount];
                    break;
                }
            }
        }
    }

    if (!found)
        printf("No intersections found.\n");

    return 0;
}
