#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#define DEFAULT_DEVICE "/dev/ttyAMA0"
#define STRINGIZE(x) #x
#define STR(x) STRINGIZE(x)

void usage();

int main(int argc, char *argv[])
{
        int daemonize = 1;
        char *device = DEFAULT_DEVICE;

        static struct option longopts[] = {
                {"version", no_argument, NULL, 'v'},
                {"no-daemon", no_argument, NULL, 'n'},
                {"device", required_argument, NULL, 'd'},
                {"help", no_argument, NULL, 'h'},
                {0, 0, 0, 0},
        };

        while(1)
        {
                int optin, curr;
                curr = getopt_long(argc, argv, "vnd:h", longopts, &optin);
                if(curr < 0)
                        break;
                switch(curr) {
                        case 'v':
                                printf(STR(VERSION) "\n");
                                break;
                        case 'n':
                                daemonize = 0;
                                break;
                        case 'd':
                                device = optarg;
                                break;
                        case 'h':
                                usage();
                                exit(0);
                                break;
                        case '?':
                                usage();
                                exit(1);
                                break;
                }
        }

        return 0;
}

void usage()
{
        printf(
                "Usage: " STR(NAME) " [OPTIONS]... [DEVICE not implemented]\n"
                "Using the GPRS/GSM modem represented by DEVICE\n"
                "tunnel HTTP traffic over SMS from requests to the modem\n\n"
                "Options,\n"
                "\t--version, -v\t Print the package version and exit.\n"
                "\t--no-daemon, -n\t Do not attempt to daemonize the process\n" 
                "\t--device, -d\t The device file for the GPRS/GSM module, defaults to \"/dev/ttyAMA0\"\n" 
                "\t--help, -h\t Print this message and exit\n" 
                "Exit status,\n"
                "\t 0, Normal operation\n"
                "\t 1, Minor error\n"
                "\t 2, Major Error\n"
        );
}
