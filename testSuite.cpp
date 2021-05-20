#include <gtest/gtest.h> // googletest header file
#include <ps2sdkapi.h>
#include <sys/times.h>
#include <string>
using std::string;

const char *actualValTrue  = "hello gtest";
const char *actualValFalse = "hello world";
const char *expectVal      = "hello gtest";

time_t getCurrentTime() {
    time_t tmp = time(NULL);
    return tmp;
}

int divisionTimes(int num) {
    int tmp = num;
    int res = 0;
    while (tmp > 0) {
        tmp /= 2;
        res++;
    }
    getCurrentTime();
    return std::max(res-1, 0);
}

bool printHelloWorlds(int times) {
    for (int i=0; i < times; i++) {
        printf("Hello World!\n");
    }

    getCurrentTime();
    return true;
}

// TEST(getCurrentTime, FirstTime) {
//     EXPECT_FALSE(getCurrentTime() == 0);
// }

TEST(StrCompare, CStrEqual) {
    EXPECT_STREQ(expectVal, actualValTrue);
}

TEST(StrCompare, CStrNotEqual) {
    EXPECT_STRNE(expectVal, actualValFalse);
}

TEST(DivisionTime, 1073741824) {
    EXPECT_DOUBLE_EQ(divisionTimes(1073741824), 30);
}

// TEST(PrintHelloWorld, 100_Times) {
//     EXPECT_TRUE(printHelloWorlds(100));
// }

// TEST(PrintHelloWorld, 1000_Times) {
//     EXPECT_TRUE(printHelloWorlds(1000));
// }

// TEST(getCurrentTime, SecondTime) {
//     EXPECT_FALSE(getCurrentTime() == 0);
// }

TEST(Sleep, 5_seconds) {
    time_t start = getCurrentTime();
    sleep(5);
    time_t end = getCurrentTime();
    EXPECT_TRUE(end > start);
}

TEST(Sleep, 5000_usleep_noDetected) {
    time_t start = getCurrentTime();
    usleep(5000);
    time_t end = getCurrentTime();
    EXPECT_TRUE(end - 1 <= start);
}

TEST(Sleep, 5000_usleep_Detected) {
    struct tms t;	/* the time values will be placed into this struct */
    times(&t);
    clock_t start = t.tms_utime;
    usleep(5000);
    times(&t);
    clock_t end = t.tms_utime;
    EXPECT_DOUBLE_EQ(end - 5, start);
}

TEST(TimePerformance, times_RepetitionsXSecond) {
    struct tms t;	/* the time values will be placed into this struct */
    times(&t);
    clock_t start = t.tms_utime;
    clock_t end = start;
    int ocurrences = 1;
    while (end < start + 1000)
    {
        times(&t);
        end = t.tms_utime;
        ocurrences++;
    }

    printf("Number of called of times function per second %i\n", ocurrences);
    EXPECT_DOUBLE_EQ(end - 1000, start);
}

TEST(TimePerformance, time_RepetitionsXSecond) {
    time_t start = time(NULL);
    time_t end = start;
    /* Finish the current second before start the test */
    while(end == start) {
        end = time(NULL);
    }

    /* Now we are at the beggining of the new second, we can count properly */
    start = time(NULL);
    end = start;
    int ocurrences = 1;
    while(end == start) {
        end = time(NULL);
        ocurrences++;
    }

    printf("Number of called of time function per second %i\n", ocurrences);
    EXPECT_DOUBLE_EQ(end - 1, start);
}
