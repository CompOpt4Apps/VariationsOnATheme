#
#   This file is part of MiniFluxDiv.

#   MiniFluxDiv is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   any later version.

#   MiniFluxDiv is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with MiniFluxDiv. If not, see <http://www.gnu.org/licenses/>.
#

__author__ = 'edavis'

import csv

class CsvFile(object):
    def __init__(self, path='', readNow=False):
        self.path = path
        self.columns = []
        self.items = []

        if readNow:
            self.read()


    def dictToRow(self, dict):
        cols = self.columns
        row = []

        for i in range(0, len(cols)):
            val = dict[cols[i]]
            row.append(val)

        return row


    def filter(self, key='', val=''):
        subset = []
        for item in self.items:
            if key in item and (len(val) < 1 or item[key] == val):
                subset.append(item)
        return subset


    def read(self):
        file = open(self.path, 'r')
        reader = csv.reader(file)

        header = []
        for row in reader:
            header = row
            break

        self.columns = header

        for row in reader:
            # Create a dict for each subsequent item...
            dict = {}
            for i in range(0, len(row)):
                column = header[i]
                dict[column] = row[i]

            self.items.append(dict)

        file.close()

    def write(self):
        f = open(self.path, 'w')

        w = csv.writer(f)
        w.writerow(self.columns)

        for item in self.items:
            row = self.dictToRow(item)
            w.writerow(row)

        f.close()

