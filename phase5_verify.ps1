Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

$hostChecksPassed = $true

function Invoke-HostCheck {
    param(
        [string]$Name,
        [scriptblock]$Action
    )

    try {
        & $Action
        Write-Host "[OK] $Name"
    }
    catch {
        $global:hostChecksPassed = $false
        Write-Host "[FAIL] $Name"
        Write-Host $_.Exception.Message
    }
}

$tempDir = Join-Path $root ".phase5_verify_tmp"
if (Test-Path $tempDir) { Remove-Item -Recurse -Force $tempDir }
New-Item -ItemType Directory -Path $tempDir | Out-Null

$arduinoHeader = @'
#pragma once
#include <stdint.h>
#define DEVICE_DISCONNECTED_C (-127.0f)
class SerialClass {
public:
    void println(const char*) {}
    void print(const char*) {}
    void printf(const char*, ...) {}
};
inline SerialClass Serial;
inline int analogRead(int) { return 1024; }
inline unsigned long millis() { return 0; }
inline void delay(unsigned long) {}
inline void pinMode(int, int) {}
'@
Set-Content -Path (Join-Path $tempDir "Arduino.h") -Value $arduinoHeader -Encoding ascii

$espSleepHeader = @'
#pragma once
#include <stdint.h>
inline void esp_sleep_enable_timer_wakeup(uint64_t) {}
inline void esp_deep_sleep_start() {}
'@
Set-Content -Path (Join-Path $tempDir "esp_sleep.h") -Value $espSleepHeader -Encoding ascii

$phase5Harness = @'
#include <iostream>
#include "Arduino.h"
#include "subnode/sensors.h"
#include "subnode/power.h"
#include "gateway/gateway_web_config.h"
#include "shared/emberlink_shared.h"

int main() {
    const float temp = emberlink::subnode::readTemperatureC(4);
    if (temp != DEVICE_DISCONNECTED_C) {
        std::cout << "TEMP_GUARD_BAD=" << temp << std::endl;
        return 10;
    }

    if (!emberlink::subnode::shouldRunSelfTest(14, 7)) {
        std::cout << "SELFTEST_GUARD_BAD" << std::endl;
        return 20;
    }

    emberlink::gateway::GatewayWebConfig portal;
    portal.begin("EMBERLINK-GATEWAY", "emberlink123");
    if (!portal.isPortalReady()) {
        std::cout << "PORTAL_BAD" << std::endl;
        return 30;
    }

    emberlink::MeshAlertPacket packet;
    packet.magic = emberlink::RTC_MAGIC;
    packet.version = emberlink::MESH_PACKET_VERSION;
    packet.nodeId[0] = 'N';
    packet.nodeId[1] = '1';
    packet.nodeId[2] = '\0';
    packet.eventType = emberlink::EventType::ALERT;
    packet.alarmLevel = emberlink::AlarmLevel::LEVEL_2;
    packet.smokeValue = 220;
    packet.temperatureC = 41.5f;
    if (!emberlink::isValidPacket(packet)) {
        std::cout << "PACKET_VALIDATION_BAD" << std::endl;
        return 40;
    }

    std::cout << "PHASE5_GAPS_FIXED_OK" << std::endl;
    return 0;
}
'@
Set-Content -Path (Join-Path $tempDir "phase5_harness.cpp") -Value $phase5Harness -Encoding ascii

Invoke-HostCheck -Name "Phase 5 host-side compile validation" -Action {
    $target = Join-Path $tempDir "phase5_harness.exe"
    g++ -std=c++17 -Wall -Wextra -pedantic -I"$tempDir" -I"$root" (Join-Path $tempDir "phase5_harness.cpp") subnode/sensors.cpp subnode/power.cpp gateway/gateway_web_config.cpp -o $target
    if ($LASTEXITCODE -ne 0) { throw "g++ compile failed" }
    Unblock-File -Path $target -ErrorAction SilentlyContinue
    & $target
    if ($LASTEXITCODE -ne 0) { throw "Phase 5 harness returned failure" }
}

if ($hostChecksPassed) {
    Write-Host "PHASE5_HOST_VALIDATION_OK"
    Write-Host "Hardware-attach validation is still required on the actual ESP32 gateway/sub-node boards."
}
else {
    throw "Phase 5 host validation failed"
}

Remove-Item -Recurse -Force $tempDir -ErrorAction SilentlyContinue
