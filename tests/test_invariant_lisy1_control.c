#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Forward declaration of the function under test */
extern int lisy1_control_update(const char *update_file);

START_TEST(test_update_file_integrity_boundary)
{
    /* Invariant: Update mechanism must reject or safely handle tampered/malicious archives
       without executing arbitrary code. The function must validate archive integrity
       before unpacking and executing install.sh */
    
    const char *payloads[] = {
        /* Valid case: legitimate tar.gz header signature */
        "\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x00",
        /* Exploit: tar with embedded shell metacharacters in filename */
        "$(malicious_command).tar.gz",
        /* Boundary: empty/truncated archive */
        "",
        /* Boundary: non-tar file masquerading as update */
        "#!/bin/bash\nrm -rf /\n",
        /* Exploit: path traversal in archive */
        "../../../etc/passwd.tar.gz"
    };
    
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);
    
    for (int i = 0; i < num_payloads; i++) {
        /* Create temporary test file with adversarial payload */
        char tmpfile[256];
        snprintf(tmpfile, sizeof(tmpfile), "/tmp/lisy_test_%d.tgz", i);
        
        int fd = open(tmpfile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        ck_assert_int_ge(fd, 0);
        
        ssize_t written = write(fd, payloads[i], strlen(payloads[i]));
        close(fd);
        
        /* Security property: function must not execute arbitrary commands
           from tampered archives. Return value should indicate failure or
           safe rejection for invalid/malicious inputs */
        int result = lisy1_control_update(tmpfile);
        
        /* Assert: Invalid archives must not succeed (result != 0 or safe handling) */
        if (i > 0) {  /* Skip first valid case from strict failure check */
            ck_assert_msg(result != 0 || written < 10,
                "Update function accepted suspicious payload at index %d", i);
        }
        
        unlink(tmpfile);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("UpdateIntegrity");

    tcase_add_test(tc_core, test_update_file_integrity_boundary);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}