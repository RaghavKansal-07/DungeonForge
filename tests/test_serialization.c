#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "serialization.h"

#define TEST_FILE "tests/test_serialization.dat"

static int tests_run = 0;
static int tests_passed = 0;


static void Test_Assert(
    bool condition,
    const char *test_name
)
{
    tests_run++;

    if (condition)
    {
        tests_passed++;

        printf(
            "[PASS] %s\n",
            test_name
        );
    }
    else
    {
        printf(
            "[FAIL] %s\n",
            test_name
        );
    }
}


/*
 * ---------------------------------------------------------
 * INITIALIZATION
 * ---------------------------------------------------------
 */

static void Test_Initialization(void)
{
    remove(TEST_FILE);

    int value = 12345;

    Test_Assert(
        !Serialization_Read(
            TEST_FILE,
            &value,
            sizeof(value)
        ),
        "Reading missing file fails safely"
    );
}


/*
 * ---------------------------------------------------------
 * WRITE / READ ROUND TRIP
 * ---------------------------------------------------------
 */

static void Test_WriteReadRoundTrip(void)
{
    const char *message =
        "DungeonForge Serialization Test";

    char buffer[64] = {0};

    bool write_success =
        Serialization_Write(
            TEST_FILE,
            message,
            strlen(message) + 1
        );

    bool read_success =
        Serialization_Read(
            TEST_FILE,
            buffer,
            strlen(message) + 1
        );

    Test_Assert(
        write_success,
        "Serialization_Write succeeds"
    );

    Test_Assert(
        read_success,
        "Serialization_Read succeeds"
    );

    Test_Assert(
        strcmp(message, buffer) == 0,
        "Written data matches read data"
    );
}


/*
 * ---------------------------------------------------------
 * MULTI-BYTE DATA
 * ---------------------------------------------------------
 */

static void Test_MultipleByteData(void)
{
    unsigned char write_data[] =
    {
        0x00,
        0x01,
        0x7F,
        0x80,
        0xAA,
        0x55,
        0xFF
    };

    unsigned char read_data[sizeof(write_data)] = {0};

    Test_Assert(
        Serialization_Write(
            TEST_FILE,
            write_data,
            sizeof(write_data)
        ),
        "Binary byte data writes successfully"
    );

    Test_Assert(
        Serialization_Read(
            TEST_FILE,
            read_data,
            sizeof(read_data)
        ),
        "Binary byte data reads successfully"
    );

    Test_Assert(
        memcmp(
            write_data,
            read_data,
            sizeof(write_data)
        ) == 0,
        "Binary byte data is preserved exactly"
    );
}


/*
 * ---------------------------------------------------------
 * INVALID ARGUMENTS
 * ---------------------------------------------------------
 */

static void Test_InvalidArguments(void)
{
    int value = 42;

    Test_Assert(
        !Serialization_Write(
            NULL,
            &value,
            sizeof(value)
        ),
        "Write rejects NULL filename"
    );

    Test_Assert(
        !Serialization_Write(
            TEST_FILE,
            NULL,
            sizeof(value)
        ),
        "Write rejects NULL data"
    );

    Test_Assert(
        !Serialization_Write(
            TEST_FILE,
            &value,
            0
        ),
        "Write rejects zero-size data"
    );

    Test_Assert(
        !Serialization_Read(
            NULL,
            &value,
            sizeof(value)
        ),
        "Read rejects NULL filename"
    );

    Test_Assert(
        !Serialization_Read(
            TEST_FILE,
            NULL,
            sizeof(value)
        ),
        "Read rejects NULL data"
    );

    Test_Assert(
        !Serialization_Read(
            TEST_FILE,
            &value,
            0
        ),
        "Read rejects zero-size data"
    );
}


/*
 * ---------------------------------------------------------
 * TRUNCATED FILE
 * ---------------------------------------------------------
 */

static void Test_TruncatedFile(void)
{
    unsigned char write_data[] =
    {
        10,
        20,
        30,
        40,
        50,
        60,
        70,
        80
    };

    unsigned char read_data[sizeof(write_data)] = {0};

    FILE *file =
        fopen(
            TEST_FILE,
            "wb"
        );

    bool file_created =
        file != NULL;

    if (file != NULL)
    {
        fwrite(
            write_data,
            1,
            3,
            file
        );

        fclose(file);
    }

    Test_Assert(
        file_created,
        "Truncated test file created"
    );

    Test_Assert(
        !Serialization_Read(
            TEST_FILE,
            read_data,
            sizeof(read_data)
        ),
        "Read rejects truncated data"
    );
}


/*
 * ---------------------------------------------------------
 * CLEANUP
 * ---------------------------------------------------------
 */

static void Test_Cleanup(void)
{
    remove(TEST_FILE);

    FILE *file =
        fopen(
            TEST_FILE,
            "rb"
        );

    Test_Assert(
        file == NULL,
        "Temporary serialization file removed"
    );

    if (file != NULL)
        fclose(file);
}


/*
 * ---------------------------------------------------------
 * MAIN TEST RUNNER
 * ---------------------------------------------------------
 */

int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf(" DungeonForge - Serialization Tests\n");
    printf("========================================\n\n");

    Test_Initialization();

    Test_WriteReadRoundTrip();

    Test_MultipleByteData();

    Test_InvalidArguments();

    Test_TruncatedFile();

    Test_Cleanup();

    printf("\n========================================\n");
    printf(
        "Tests: %d | Passed: %d | Failed: %d\n",
        tests_run,
        tests_passed,
        tests_run - tests_passed
    );
    printf("========================================\n\n");

    return tests_run == tests_passed ? 0 : 1;
}
