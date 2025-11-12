#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

struct Point {
    double x;
    double y;
};

struct Segment {
    int id;
    struct Point p1;
    struct Point p2;
};

struct Event {
    double y;
    int segment_index;
    bool type;
};

struct Segment *create_segment_list();
struct Event *make_event_list(struct Segment *segment_list);

int main () {
    struct Segment *segment_list = create_segment_list();
    return 0;
}

struct Segment *create_segment_list() {
    int points[][4] = {{0, 0, 4, 4},
                       {0, 4, 4, 0}};

    int n = sizeof(points) / sizeof(points[0]);

    struct Segment *s_list = calloc(sizeof(struct Segment), n);

    for (int i=0; i<n; i++) {
        s_list[i].id = i;
        s_list[i].p1.x = points[i][0];
        s_list[i].p1.y = points[i][1];
        s_list[i].p2.x = points[i][0];
        s_list[i].p2.y = points[i][1];
    }

    return s_list;
}

struct Event *make_event_list(struct Segment *segment_list);
