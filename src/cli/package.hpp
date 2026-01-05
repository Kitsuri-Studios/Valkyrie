/*
 * Copyright 2026 Kitsuri Studios
 * Developed by Mostafizur Rahman (aeticusdev)
 *
 * SUMMARY (BSD 3-Clause License):
 *  You may use, copy, modify, and distribute this software
 *  You may use it for commercial and private purposes
 *  You must include this copyright notice and license text
 *  You may NOT use the project name or contributors to endorse derived products
 *  No warranty or liability is provided by the authors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "utils.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

inline void create_deb_package(const std::string& app_name, const std::string& version) {
    std::cout << "Creating .deb package...\n" << std::endl;
    
    std::string pkg_dir = app_name + "_" + version + "_amd64";
    std::string bin_dir = pkg_dir + "/usr/local/bin";
    std::string share_dir = pkg_dir + "/usr/share/applications";
    std::string icon_dir = pkg_dir + "/usr/share/icons/hicolor/256x256/apps";
    std::string debian_dir = pkg_dir + "/DEBIAN";
    
    fs::create_directories(bin_dir);
    fs::create_directories(share_dir);
    fs::create_directories(icon_dir);
    fs::create_directories(debian_dir);
    
    exec_cmd("cp app " + bin_dir + "/" + app_name, nullptr);
    
    std::string desktop = "[Desktop Entry]\n";
    desktop += "Name=" + app_name + "\n";
    desktop += "Exec=/usr/local/bin/" + app_name + "\n";
    desktop += "Type=Application\n";
    desktop += "Categories=Utility;\n";
    if (fs::exists("icon.png")) {
        desktop += "Icon=" + app_name + "\n";
        exec_cmd("cp icon.png " + icon_dir + "/" + app_name + ".png", nullptr);
    }
    write_file(share_dir + "/" + app_name + ".desktop", desktop);
    
    std::string control = "Package: " + app_name + "\n";
    control += "Version: " + version + "\n";
    control += "Architecture: amd64\n";
    control += "Maintainer: developer\n";
    control += "Description: " + app_name + " desktop app\n";
    control += "Depends: libgtk-3-0, libwebkit2gtk-4.0-37\n";
    write_file(debian_dir + "/control", control);
    
    int exit_code = 0;
    std::string output = exec_cmd("dpkg-deb --build " + pkg_dir, &exit_code);
    
    if (exit_code == 0) {
        std::cout << "\nCreated: " << pkg_dir << ".deb" << std::endl;
        exec_cmd("rm -rf " + pkg_dir, nullptr);
    } else {
        print_error("DEB packaging failed", output);
    }
}

inline void create_rpm_package(const std::string& app_name, const std::string& version) {
    std::cout << "Creating .rpm package...\n" << std::endl;
    
    int check = system("which rpmbuild > /dev/null 2>&1");
    if (check != 0) {
        print_error("rpmbuild not found", "Install: sudo dnf install rpm-build");
        return;
    }
    
    std::string home = getenv("HOME");
    std::string rpmbuild = home + "/rpmbuild";
    
    fs::create_directories(rpmbuild + "/BUILD");
    fs::create_directories(rpmbuild + "/RPMS");
    fs::create_directories(rpmbuild + "/SOURCES");
    fs::create_directories(rpmbuild + "/SPECS");
    fs::create_directories(rpmbuild + "/SRPMS");
    
    std::string spec = "Name: " + app_name + "\n";
    spec += "Version: " + version + "\n";
    spec += "Release: 1%{?dist}\n";
    spec += "Summary: " + app_name + " desktop app\n";
    spec += "License: MIT\n";
    spec += "\n%description\n" + app_name + " desktop application\n";
    spec += "\n%install\n";
    spec += "mkdir -p %{buildroot}/usr/local/bin\n";
    spec += "cp " + fs::current_path().string() + "/app %{buildroot}/usr/local/bin/" + app_name + "\n";
    spec += "\n%files\n";
    spec += "/usr/local/bin/" + app_name + "\n";
    
    write_file(rpmbuild + "/SPECS/" + app_name + ".spec", spec);
    
    int exit_code = 0;
    std::string output = exec_cmd("rpmbuild -bb " + rpmbuild + "/SPECS/" + app_name + ".spec", &exit_code);
    
    if (exit_code == 0) {
        std::string find_cmd = "find " + rpmbuild + "/RPMS -name '*.rpm'";
        std::string rpm_path = exec_cmd(find_cmd, nullptr);
        rpm_path.erase(rpm_path.find_last_not_of("\n\r") + 1);
        
        if (!rpm_path.empty()) {
            std::string rpm_name = app_name + "-" + version + "-1.x86_64.rpm";
            exec_cmd("cp " + rpm_path + " " + rpm_name, nullptr);
            std::cout << "\nCreated: " << rpm_name << std::endl;
        }
    } else {
        print_error("RPM packaging failed", output);
    }
}

inline void create_arch_package(const std::string& app_name, const std::string& version) {
    std::cout << "Creating Arch package...\n" << std::endl;
    
    std::string pkgbuild = "# maintainer: developer\n";
    pkgbuild += "pkgname=" + app_name + "\n";
    pkgbuild += "pkgver=" + version + "\n";
    pkgbuild += "pkgrel=1\n";
    pkgbuild += "pkgdesc=\"" + app_name + " desktop app\"\n";
    pkgbuild += "arch=('x86_64')\n";
    pkgbuild += "depends=('gtk3' 'webkit2gtk')\n";
    pkgbuild += "\n";
    pkgbuild += "package() {\n";
    pkgbuild += "  install -Dm755 \"$srcdir/../app\" \"$pkgdir/usr/local/bin/" + app_name + "\"\n";
    
    if (fs::exists("icon.png")) {
        pkgbuild += "  install -Dm644 \"$srcdir/../icon.png\" \"$pkgdir/usr/share/icons/hicolor/256x256/apps/" + app_name + ".png\"\n";
    }
    
    pkgbuild += "  mkdir -p \"$pkgdir/usr/share/applications\"\n";
    pkgbuild += "  cat > \"$pkgdir/usr/share/applications/" + app_name + ".desktop\" << EOF\n";
    pkgbuild += "[Desktop Entry]\n";
    pkgbuild += "Name=" + app_name + "\n";
    pkgbuild += "Exec=/usr/local/bin/" + app_name + "\n";
    pkgbuild += "Type=Application\n";
    pkgbuild += "Categories=Utility;\n";
    if (fs::exists("icon.png")) {
        pkgbuild += "Icon=" + app_name + "\n";
    }
    pkgbuild += "EOF\n";
    pkgbuild += "}\n";
    
    write_file("PKGBUILD", pkgbuild);
    
    int exit_code = 0;
    std::string output = exec_cmd("makepkg -f", &exit_code);
    
    if (exit_code == 0) {
        std::string find_cmd = "ls -1 " + app_name + "-" + version + "*.pkg.tar.zst 2>/dev/null | head -1";
        std::string pkg_file = exec_cmd(find_cmd, nullptr);
        pkg_file.erase(pkg_file.find_last_not_of("\n\r") + 1);
        
        if (!pkg_file.empty()) {
            std::cout << "\nCreated: " << pkg_file << std::endl;
        }
        
        exec_cmd("rm -rf src pkg PKGBUILD", nullptr);
    } else {
        print_error("Arch packaging failed", output);
        exec_cmd("rm -f PKGBUILD", nullptr);
    }
}

inline void create_macos_app(const std::string& app_name, const std::string& version) {
    std::cout << "Creating .app bundle...\n" << std::endl;
    
    std::string bundle = app_name + ".app";
    std::string contents = bundle + "/Contents";
    std::string macos_dir = contents + "/MacOS";
    std::string resources = contents + "/Resources";
    
    fs::create_directories(macos_dir);
    fs::create_directories(resources);
    
    exec_cmd("cp app " + macos_dir + "/" + app_name, nullptr);
    
    std::string plist = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    plist += "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
    plist += "<plist version=\"1.0\">\n<dict>\n";
    plist += "  <key>CFBundleName</key>\n  <string>" + app_name + "</string>\n";
    plist += "  <key>CFBundleDisplayName</key>\n  <string>" + app_name + "</string>\n";
    plist += "  <key>CFBundleIdentifier</key>\n  <string>com.valkyrie." + app_name + "</string>\n";
    plist += "  <key>CFBundleVersion</key>\n  <string>" + version + "</string>\n";
    plist += "  <key>CFBundleExecutable</key>\n  <string>" + app_name + "</string>\n";
    plist += "  <key>CFBundlePackageType</key>\n  <string>APPL</string>\n";
    plist += "</dict>\n</plist>\n";
    
    write_file(contents + "/Info.plist", plist);
    
    if (fs::exists("icon.icns")) {
        exec_cmd("cp icon.icns " + resources + "/" + app_name + ".icns", nullptr);
    }
    
    std::cout << "\nCreated: " << bundle << std::endl;
    
    std::cout << "Creating DMG..." << std::endl;
    std::string dmg_name = app_name + "-" + version + ".dmg";
    int exit_code = 0;
    std::string output = exec_cmd("hdiutil create -volname " + app_name + " -srcfolder " + bundle + " -ov -format UDZO " + dmg_name, &exit_code);
    
    if (exit_code == 0) {
        std::cout << "Created: " << dmg_name << std::endl;
    }
}

inline void create_windows_installer(const std::string& app_name, const std::string& version, std::function<void(const std::string&)> build_fn) {
    std::cout << "Creating Windows package...\n" << std::endl;
    
    if (!fs::exists("app.exe")) {
        std::cout << "No app.exe found, building for Windows..." << std::endl;
        build_fn("windows");
        
        if (!fs::exists("app.exe")) {
            print_error("Windows build failed", "Verify mingw-w64 is installed.");
            return;
        }
    }
    
    std::string zip_name = app_name + "-" + version + "-windows.zip";
    
    std::string pkg_dir = "_win_pkg";
    fs::create_directories(pkg_dir);
    
    fs::copy("app.exe", pkg_dir + "/" + app_name + ".exe", fs::copy_options::overwrite_existing);
    
    std::string readme = app_name + " v" + version + "\n";
    readme += std::string(app_name.length() + version.length() + 3, '=') + "\n\n";
    readme += "Double-click " + app_name + ".exe to run.\n\n";
    readme += "Built with Valkyrie - https://github.com/user/valkyrie\n";
    write_file(pkg_dir + "/README.txt", readme);
    
    int exit_code = 0;
    std::string output = exec_cmd("cd " + pkg_dir + " && zip -r ../" + zip_name + " .", &exit_code);
    
    fs::remove_all(pkg_dir);
    
    if (exit_code != 0) {
        print_error("Failed to create ZIP archive", output);
        return;
    }
    
    std::cout << "Created: " << zip_name << std::endl;
    
    auto size = fs::file_size(zip_name);
    std::cout << "Size: " << (size / 1024 / 1024) << " MB" << std::endl;
}

inline void package_app(const std::string& target_platform, std::function<void(const std::string&)> build_fn) {
    if (!fs::exists("app")) {
        print_error("No application binary found", "Run 'valkyrie build' first.");
        return;
    }
    
    std::string app_name = "app";
    std::string version = "1.0.0";
    
    if (fs::exists("package.json")) {
        std::string pkg_json = read_file("package.json");
        
        auto name_pos = pkg_json.find("\"name\"");
        if (name_pos != std::string::npos) {
            auto colon = pkg_json.find(":", name_pos);
            auto quote1 = pkg_json.find("\"", colon + 1);
            auto quote2 = pkg_json.find("\"", quote1 + 1);
            if (quote1 != std::string::npos && quote2 != std::string::npos) {
                app_name = pkg_json.substr(quote1 + 1, quote2 - quote1 - 1);
            }
        }
        
        auto ver_pos = pkg_json.find("\"version\"");
        if (ver_pos != std::string::npos) {
            auto colon = pkg_json.find(":", ver_pos);
            auto quote1 = pkg_json.find("\"", colon + 1);
            auto quote2 = pkg_json.find("\"", quote1 + 1);
            if (quote1 != std::string::npos && quote2 != std::string::npos) {
                version = pkg_json.substr(quote1 + 1, quote2 - quote1 - 1);
            }
        }
    }
    
    std::cout << "Packaging: " << app_name << " v" << version << "\n" << std::endl;
    
    std::string platform = target_platform.empty() ? get_platform() : target_platform;
    
    if (platform == "linux") {
        std::cout << "Target platform: Linux" << std::endl;
        std::cout << "\nPackage type:\n";
        std::cout << "  1) .deb (Debian/Ubuntu)\n";
        std::cout << "  2) .rpm (Fedora/RHEL)\n";
        std::cout << "  3) .pkg.tar.zst (Arch)\n";
        std::cout << "  4) All formats\n";
        std::cout << "Choice [1]: ";
        
        std::string choice;
        std::getline(std::cin, choice);
        if (choice.empty()) choice = "1";
        
        if (choice == "1" || choice == "4") {
            create_deb_package(app_name, version);
        }
        if (choice == "2" || choice == "4") {
            create_rpm_package(app_name, version);
        }
        if (choice == "3" || choice == "4") {
            create_arch_package(app_name, version);
        }
    } else if (platform == "macos") {
        create_macos_app(app_name, version);
    } else if (platform == "windows") {
        create_windows_installer(app_name, version, build_fn);
    } else {
        print_error("Unsupported platform", platform);
    }
}
