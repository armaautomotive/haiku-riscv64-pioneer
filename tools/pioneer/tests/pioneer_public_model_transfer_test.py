#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from pioneer_public_model_transfer import byte_range, parent_range, transfer_ranges, Handler, ENDPOINT
from types import SimpleNamespace


class RangeTests(unittest.TestCase):
    def test_partitions_cover_exact_suffix(self):
        for size in (17, 18973870528):
            for start in (0, 1, size//2):
                for count in range(1, min(16,size-start)+1):
                    ranges = transfer_ranges(start,count,size)
                    self.assertLessEqual(len(ranges),count)
                    at = start
                    for first,last in ranges:
                        self.assertEqual(first,at)
                        self.assertLessEqual(first,last)
                        self.assertLess(last,size)
                        at=last+1
                    self.assertEqual(at,size)
        for args in ((-1,8,100),(100,8,100),(0,0,100),(0,17,100),(99,2,100)):
            with self.assertRaises(ValueError):
                transfer_ranges(*args)

    def test_peer_and_path_refusal_before_any_file_access(self):
        for peer,path in (("127.0.0.1",ENDPOINT),("10.0.0.211",ENDPOINT),
                          ("10.0.0.247","/"),("10.0.0.247","/../"),
                          ("10.0.0.247",ENDPOINT+"?x=1")):
            errors=[]
            request=SimpleNamespace(client_address=(peer,1),path=path,send_error=errors.append)
            Handler.transfer(request,False)
            self.assertEqual(errors,[403])

    def test_only_exact_declared_range_or_its_resume_suffix(self):
        declared = transfer_ranges(3046275824, 8, 18973870528)
        self.assertEqual(len(declared), 8)
        for first, last in declared:
            for resumed in (first, first+1, (first+last)//2, last):
                self.assertEqual(parent_range(resumed, last, declared), (first, last))
            for start, end in ((first, last-1), (first, last+1), (first-1, last),
                               (last+1, last), (first, 18973870527+1)):
                with self.assertRaises(ValueError):
                    parent_range(start, end, declared)
        for first, last in ((0, 18973870527), (0, 3046275823),
                            (3046275824, 18973870527), (18973870528, 18973870528)):
            with self.assertRaises(ValueError):
                parent_range(first, last, declared)

    def test_full_and_partial(self):
        self.assertEqual(byte_range(None, 100), (0, 99, False))
        self.assertEqual(byte_range("bytes=20-", 100), (20, 99, True))
        self.assertEqual(byte_range("bytes=0-0", 100), (0, 0, True))
        self.assertEqual(byte_range("bytes=0-99", 100), (0, 99, True))

    def test_invalid(self):
        for value in ("", "bytes=-1", "bytes=10-9", "bytes=100-", "bytes=0-100",
                      "bytes=0-1,2-3", "items=0-1", "bytes=+1-2", "bytes=1-2\n", "bytes="+"0"*64+"-"):
            with self.assertRaises(ValueError):
                byte_range(value, 100)


if __name__ == "__main__":
    unittest.main()
