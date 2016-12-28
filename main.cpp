#include <iostream>
#include <map>
#include <iomanip>
#include "SpeedTest.h"
#include "TestConfigTemplate.h"
#include "CmdOptions.h"


void banner(){
    std::cout << "SpeedTest++ version " << SpeedTest_VERSION_MAJOR << "." << SpeedTest_VERSION_MINOR << std::endl;
    std::cout << "Speedtest.net command line interface" << std::endl;
    std::cout << "Info: " << SpeedTest_HOME_PAGE << std::endl;
    std::cout << "Author: " << SpeedTest_AUTHOR << std::endl;
}

void usage(const char* name){
    std::cerr << "Usage: " << name << " ";
    std::cerr << " [--latency] [--quality] [--download] [--upload] [--share] [--help]\n"
            "      [--test-server host:port] [--quality-server host:port] [--output verbose|text]\n";
    std::cerr << "optional arguments:" << std::endl;
    std::cerr << "  --help                      Show this message and exit\n";
    std::cerr << "  --latency                   Perform latency test only\n";
    std::cerr << "  --quality                   Perform quality test only. It includes latency test\n";
    std::cerr << "  --download                  Perform download test only. It includes latency test\n";
    std::cerr << "  --upload                    Perform upload test only. It includes latency test\n";
    std::cerr << "  --share                     Generate and provide a URL to the speedtest.net share results image\n";
    std::cerr << "  --test-server host:port     Run speed test against a specific server\n";
    std::cerr << "  --ping-server host:port  Run line quality test against a specific server\n";
    std::cerr << "  --output verbose|text       Set output type. Default: verbose\n";
}

int main(const int argc, const char **argv) {

    ProgramOptions programOptions;

    if (!ParseOptions(argc, argv, programOptions)){
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (programOptions.output_type == OutputType::verbose){
        banner();
        std::cout << std::endl;
    }


    if (programOptions.help) {
        usage(argv[0]);
        return EXIT_SUCCESS;
    }


    auto sp = SpeedTest(SPEED_TEST_MIN_SERVER_VERSION);
    IPInfo info;
    ServerInfo serverInfo;
    ServerInfo serverQualityInfo;

    if (!sp.ipInfo(info)){
        std::cerr << "Unable to retrieve your IP info. Try again later" << std::endl;
        return EXIT_FAILURE;
    }

    if (programOptions.output_type == OutputType::verbose){
        std::cout << "IP: " << info.ip_address
                  << " ( " << info.isp << " ) "
                  << "Location: [" << info.lat << ", " << info.lon << "]" << std::endl;
    } else {
        std::cout << "IP=" << info.ip_address << std::endl;
        std::cout << "IP_LAT=" << info.lat << std::endl;
        std::cout << "IP_LON=" << info.lon << std::endl;
        std::cout << "PROVIDER=" << info.isp << std::endl;
    }


    if (programOptions.selected_server.empty()){
        if (programOptions.output_type == OutputType::verbose)
            std::cout << "Finding fastest server... " << std::flush;
        auto serverList = sp.serverList();
        if (serverList.empty()){
            std::cerr << "Unable to download server list. Try again later" << std::endl;
            return EXIT_FAILURE;
        }

        if (programOptions.output_type == OutputType::verbose)
            std::cout << serverList.size() << " Servers online" << std::endl;


        serverInfo = sp.bestServer(10, [&programOptions](bool success) {
            if (programOptions.output_type == OutputType::verbose)
                std::cout << (success ? '.' : '*') << std::flush;
        });

        if (programOptions.output_type == OutputType::verbose){
            std::cout << std::endl;
            std::cout << "Server: " << serverInfo.name
                      << " by " << serverInfo.sponsor
                      << " (" << serverInfo.distance << " km from you): "
                      << sp.latency() << " ms" << std::endl;
        } else {
            std::cout << "TEST_SERVER_HOST=" << serverInfo.host << std::endl;
            std::cout << "TEST_SERVER_DISTANCE=" << serverInfo.distance << std::endl;

        }

    } else {

        serverInfo.host.append(programOptions.selected_server);
        sp.setServer(serverInfo);

        if (programOptions.output_type == OutputType::verbose)
            std::cout << "Selected server: " << serverInfo.host << std::endl;
        else {
            std::cout << "TEST_SERVER_HOST=" << serverInfo.host << std::endl;
        }
    }

    if (programOptions.output_type == OutputType::verbose)
        std::cout << "Ping: " << sp.latency() << " ms." << std::endl;
    else
        std::cout << "LATENCY=" << sp.latency() << std::endl;

    long jitter = 0;
    if (programOptions.output_type == OutputType::verbose)
        std::cout << "Jitter: " << std::flush;
    if (sp.jitter(serverInfo, jitter)){
        if (programOptions.output_type == OutputType::verbose)
            std::cout << jitter << " ms." << std::endl;
        else
            std::cout << "JITTER=" << jitter << std::endl;
    } else {
        std::cerr << "Jitter measurement is unavailable at this time." << std::endl;
    }

    if (programOptions.latency)
        return EXIT_SUCCESS;

    if (programOptions.selected_ping_server.empty()) {

        if (programOptions.output_type == OutputType::verbose)
            std::cout << "Finding fastest server for packet loss measurement... " << std::flush;

        auto serverQualityList = sp.serverQualityList();

        if (serverQualityList.empty()) {
            std::cerr << "Unable to download server list. Packet loss analysis is not available at this time"
                      << std::endl;
        } else {

            if (programOptions.output_type == OutputType::verbose)
                std::cout << serverQualityList.size() << " Ping hosts online" << std::endl;

            serverQualityInfo = sp.bestQualityServer(5, [&programOptions](bool success) {
                if (programOptions.output_type == OutputType::verbose)
                    std::cout << (success ? '.' : '*') << std::flush;
            });

            if (programOptions.output_type == OutputType::verbose){
                std::cout << std::endl;
                std::cout << "Server: " << serverQualityInfo.name
                          << " by " << serverQualityInfo.sponsor
                          << " (" << serverQualityInfo.distance << " km from you): " << std::endl;
            } else {
                std::cout << "QUALITY_SERVER_HOST=" << serverQualityInfo.linequality << std::endl;
                std::cout << "QUALITY_SERVER_DISTANCE=" << serverQualityInfo.distance << std::endl;
            }

        }
    } else {
        serverQualityInfo.linequality.append(programOptions.selected_ping_server);
        sp.setServer(serverQualityInfo, true);

        if (programOptions.output_type == OutputType::verbose)
            std::cout << "Selected quality server: " << serverQualityInfo.linequality << std::endl;
        else
            std::cout << "QUALITY_SERVER_HOST=" << serverQualityInfo.linequality << std::endl;
    }

    int ploss = 0;
    if (sp.packetLoss(serverQualityInfo, ploss)) {
        if (programOptions.output_type == OutputType::verbose)
            std::cout << "Packets loss: " << ploss << std::endl;
        else
            std::cout << "PACKET_LOSS=" << ploss << std::endl;
    }


    if (programOptions.quality)
        return EXIT_SUCCESS;

    if (programOptions.output_type == OutputType::verbose)
        std::cout << "Determine line type (" << preflightConfigDownload.concurrency << ") "  << std::flush;
    double preSpeed = 0;
    if (!sp.downloadSpeed(serverInfo, preflightConfigDownload, preSpeed, [&programOptions](bool success){
        if (programOptions.output_type == OutputType::verbose)
            std::cout << (success ? '.' : '*') << std::flush;
    })){
        std::cerr << "Pre-flight check failed." << std::endl;
        return EXIT_FAILURE;
    }

    if (programOptions.output_type == OutputType::verbose)
        std::cout << std::endl;

    TestConfig uploadConfig;
    TestConfig downloadConfig;
    testConfigSelector(preSpeed, uploadConfig, downloadConfig);

    if (programOptions.output_type == OutputType::verbose)
        std::cout << downloadConfig.label << std::endl;


    if (!programOptions.upload){
        if (programOptions.output_type == OutputType::verbose){
            std::cout << std::endl;
            std::cout << "Testing download speed (" << downloadConfig.concurrency << ") "  << std::flush;
        }

        double downloadSpeed = 0;
        if (sp.downloadSpeed(serverInfo, downloadConfig, downloadSpeed, [&programOptions](bool success){
            if (programOptions.output_type == OutputType::verbose)
                std::cout << (success ? '.' : '*') << std::flush;
        })){
            if (programOptions.output_type == OutputType::verbose){
                std::cout << std::endl;
                std::cout << "Download: ";
                std::cout << std::fixed;
                std::cout << std::setprecision(2);
                std::cout << downloadSpeed << " Mbit/s" << std::endl;
            } else {
                std::cout << "DOWNLOAD_SPEED=";
                std::cout << std::fixed;
                std::cout << std::setprecision(2);
                std::cout << downloadSpeed << std::endl;
            }

        } else {
            std::cerr << "Download test failed." << std::endl;
            return EXIT_FAILURE;
        }
    }

    if (programOptions.download)
        return EXIT_SUCCESS;

    if (programOptions.output_type == OutputType::verbose)
        std::cout << "Testing upload speed (" << uploadConfig.concurrency << ") "  << std::flush;

    double uploadSpeed = 0;
    if (sp.uploadSpeed(serverInfo, uploadConfig, uploadSpeed, [&programOptions](bool success){
        if (programOptions.output_type == OutputType::verbose)
            std::cout << (success ? '.' : '*') << std::flush;
    })){
        if (programOptions.output_type == OutputType::verbose){
            std::cout << std::endl;
            std::cout << "Upload: ";
            std::cout << std::fixed;
            std::cout << std::setprecision(2);
            std::cout << uploadSpeed << " Mbit/s" << std::endl;
        } else {
            std::cout << "UPLOAD_SPEED=";
            std::cout << std::fixed;
            std::cout << std::setprecision(2);
            std::cout << uploadSpeed << std::endl;
        }

    } else {
        std::cerr << "Upload test failed." << std::endl;
        return EXIT_FAILURE;
    }


    if (programOptions.share){
        std::string share_it;
        if (sp.share(serverInfo, share_it)) {
            if (programOptions.output_type == OutputType::verbose) {
                std::cout << "Results image: " << share_it << std::endl;
            } else {
                std::cout << "IMAGE_URL=" << share_it << std::endl;
            }
        }
    }

    return EXIT_SUCCESS;
}