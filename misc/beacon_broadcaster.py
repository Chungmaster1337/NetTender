#!/usr/bin/env python3
"""
WiFi Beacon Broadcaster
Broadcasts WiFi beacon frames with custom SSID and MAC address for security testing.

Requirements: scapy, root privileges
Usage: sudo python3 beacon_broadcaster.py -i wlan0 -s "TestAP" -m "AA:BB:CC:DD:EE:FF"

WARNING: Only use on networks you own or have explicit permission to test.
"""

import argparse
import sys
import time
import random
from scapy.all import (
    RadioTap, Dot11, Dot11Beacon, Dot11Elt,
    sendp, conf
)

def validate_mac(mac):
    """Validate MAC address format."""
    parts = mac.split(':')
    if len(parts) != 6:
        return False
    try:
        return all(0 <= int(part, 16) <= 255 for part in parts)
    except ValueError:
        return False

def generate_random_mac():
    """Generate a random MAC address."""
    return ':'.join(['%02x' % random.randint(0, 255) for _ in range(6)])

def create_beacon_frame(ssid, bssid, channel=1, interval=100):
    """
    Create a WiFi beacon frame.

    Args:
        ssid: Network name to broadcast
        bssid: MAC address of the fake AP
        channel: WiFi channel (1-14)
        interval: Beacon interval in milliseconds

    Returns:
        Scapy packet ready to send
    """
    # RadioTap header
    dot11 = Dot11(
        type=0,           # Management frame
        subtype=8,        # Beacon subtype
        addr1='ff:ff:ff:ff:ff:ff',  # Destination (broadcast)
        addr2=bssid,      # Source (our fake AP)
        addr3=bssid       # BSSID (our fake AP)
    )

    # Beacon frame
    beacon = Dot11Beacon(
        cap='ESS+privacy',  # Capabilities (ESS=infrastructure, privacy=WPA/WEP)
        timestamp=int(time.time())
    )

    # Information elements
    essid = Dot11Elt(ID='SSID', info=ssid, len=len(ssid))

    # Supported rates (basic 802.11b/g rates)
    rates = Dot11Elt(ID='Rates', info=b'\x82\x84\x8b\x96\x0c\x12\x18\x24')

    # DS Parameter set (channel)
    dsset = Dot11Elt(ID='DSset', info=bytes([channel]))

    # Traffic Indication Map
    tim = Dot11Elt(ID='TIM', info=b'\x00\x01\x00\x00')

    # Assemble the complete frame
    frame = RadioTap() / dot11 / beacon / essid / rates / dsset / tim

    return frame

def broadcast_beacon(interface, ssid, mac, channel=6, interval=0.1, count=None, verbose=True):
    """
    Broadcast beacon frames continuously.

    Args:
        interface: Network interface to use (must be in monitor mode)
        ssid: SSID to broadcast
        mac: MAC address to use
        channel: WiFi channel (1-14)
        interval: Time between beacons in seconds
        count: Number of beacons to send (None = infinite)
        verbose: Print status messages
    """
    # Set scapy to use the specified interface
    conf.iface = interface

    # Create the beacon frame
    frame = create_beacon_frame(ssid, mac, channel)

    if verbose:
        print(f"[+] Starting beacon broadcast")
        print(f"[+] Interface: {interface}")
        print(f"[+] SSID: {ssid}")
        print(f"[+] MAC: {mac}")
        print(f"[+] Channel: {channel}")
        print(f"[+] Interval: {interval}s")
        print(f"[+] Press Ctrl+C to stop\n")

    sent = 0
    try:
        while count is None or sent < count:
            sendp(frame, iface=interface, verbose=False)
            sent += 1

            if verbose and sent % 10 == 0:
                print(f"\r[*] Beacons sent: {sent}", end='', flush=True)

            time.sleep(interval)

    except KeyboardInterrupt:
        if verbose:
            print(f"\n\n[!] Stopped. Total beacons sent: {sent}")
    except PermissionError:
        print("\n[!] ERROR: Need root privileges. Run with sudo.")
        sys.exit(1)
    except OSError as e:
        print(f"\n[!] ERROR: {e}")
        print(f"[!] Make sure interface '{interface}' exists and is in monitor mode")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(
        description='Broadcast WiFi beacon frames with custom SSID and MAC address',
        epilog='Example: sudo python3 beacon_broadcaster.py -i wlan0mon -s "FreeWiFi" -m "DE:AD:BE:EF:CA:FE"'
    )

    parser.add_argument('-i', '--interface', required=True,
                        help='Wireless interface in monitor mode (e.g., wlan0mon)')

    parser.add_argument('-s', '--ssid', required=True,
                        help='SSID to broadcast (max 32 characters)')

    parser.add_argument('-m', '--mac', default=None,
                        help='MAC address to use (e.g., AA:BB:CC:DD:EE:FF). Random if not specified.')

    parser.add_argument('-c', '--channel', type=int, default=6,
                        help='WiFi channel (1-14, default: 6)')

    parser.add_argument('-t', '--interval', type=float, default=0.1,
                        help='Time between beacons in seconds (default: 0.1)')

    parser.add_argument('-n', '--count', type=int, default=None,
                        help='Number of beacons to send (default: infinite)')

    parser.add_argument('-q', '--quiet', action='store_true',
                        help='Quiet mode (minimal output)')

    args = parser.parse_args()

    # Validate SSID length
    if len(args.ssid) > 32:
        print("[!] ERROR: SSID must be 32 characters or less")
        sys.exit(1)

    # Validate or generate MAC address
    if args.mac:
        if not validate_mac(args.mac):
            print("[!] ERROR: Invalid MAC address format")
            print("[!] Expected format: AA:BB:CC:DD:EE:FF")
            sys.exit(1)
        mac = args.mac
    else:
        mac = generate_random_mac()
        if not args.quiet:
            print(f"[*] Generated random MAC: {mac}")

    # Validate channel
    if not 1 <= args.channel <= 14:
        print("[!] ERROR: Channel must be between 1 and 14")
        sys.exit(1)

    # Check if running as root
    if sys.platform.startswith('linux') and os.geteuid() != 0:
        print("[!] WARNING: This script typically requires root privileges")
        print("[!] If you encounter permission errors, run with: sudo python3 beacon_broadcaster.py ...")

    # Start broadcasting
    broadcast_beacon(
        interface=args.interface,
        ssid=args.ssid,
        mac=mac,
        channel=args.channel,
        interval=args.interval,
        count=args.count,
        verbose=not args.quiet
    )

if __name__ == '__main__':
    # Import os here to avoid issues if not needed
    import os
    main()
