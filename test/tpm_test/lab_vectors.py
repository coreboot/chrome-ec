# -*- coding: utf-8 -*-
# Copyright 2021 The ChromiumOS Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module for using lab vectors."""

from __future__ import print_function

import os

import drbg_test
import utils

class LabTest(object):
    """Base class implementing the lab vector interface.

    This is used to convert the vectors from the lab to the format required
    by the tpm tests. It also converts the results from the test to the correct
    format expected by the lab.
    """

    ALGORITHM = "algorithm"
    GROUPS = "testGroups"
    TEST_CASES = "tests"
    GROUP_ID = "tgId"
    TEST_CASE_ID = "tcId"

    def __init__(self, request_file, expected_file, result_dir=""):
        """Initialize the lab test object.

        Args:
            request_file: the test input vector json filename
            expected_file: the expected result json filename
            result_dir: directory to store the actual result vector in
        """
        self._request_file = request_file
        self._expected_file = expected_file
        self._request_vector = utils.read_vectors(self._request_file)
        if self._expected_file:
            self._expected_vector = utils.read_vectors(self._expected_file)
        else:
            self._expected_vector = None

        out_file = (os.path.basename(request_file).strip('.json') +
                    '-output.json')
        self._result_file = os.path.join(result_dir,
            os.path.basename(request_file).strip('.json') + '-output.json')
        self._result_json = None
        self._test_inputs = []

    def __str__(self):
        """Return the algorithm name from the input vector."""
        return self._request_vector[1][self.ALGORITHM]

    def get_test_inputs(self):
        """Convert the lab vectors into the format required for the test."""
        raise NotImplementedError('Algorithm needs to provide the vector processing')

    def _algo_get_formatted_results(self, results):
        """Convert the results list into the lab format."""
        raise NotImplementedError('Algorithm needs to process results list')

    def save_test_results(self, results):
        """Convert the results to the lab format and save them to a file."""
        print('Saving results in', self._result_file)
        self._result_json = utils.read_vectors(self._request_file)
        formatted_results = self._algo_get_formatted_results(results)
        self._result_json[1][self.GROUPS] = formatted_results
        utils.write_test_result_json(self._result_file, self._result_json)


class DRBGLabTest(LabTest):
    """Class implementing the lab vector interface for drbg_test.

    Convert the request vector to the test_input format from the drbg test.
    Convert the response list from the drbg test to the same format as the
    expected vectors.
    """
    RESPONSE_KEY = "returnedBits"
    RESPONSE_BITS = RESPONSE_KEY + "Len"
    NONCE = "nonce"
    PERSO = "persoString"
    INPUT_1 = "additionalInput"
    ENTROPY = "entropyInput"
    MODE = "intendedUse"
    RESEED = "reSeed"
    GENERATE = "generate"
    OTHER_INPUT = "otherInput"

    def _get_expected_response(self, group_id, case_id):
        """Return the response for the given group and test case."""
        if not self._expected_vector:
            return ""
        group = self._expected_vector[1][self.GROUPS][group_id]
        return group[self.TEST_CASES][case_id][self.RESPONSE_KEY]

    def _add_test_input(self, test_input):
        """Append the test item to the test input list."""
        self._test_inputs.append(test_input)

    def _process_test_case(self, test, response):
        """Add steps from the test case into the test_inputs list."""
        drbg_op = drbg_test.DRBG_INIT
        drbg_params = (test[self.ENTROPY], test[self.NONCE], test[self.PERSO])
        self._add_test_input((drbg_op, drbg_params))
        generate_calls = 0
        for step in test[self.OTHER_INPUT]:
            mode = step[self.MODE]
            input1 = step[self.INPUT_1]
            entropy = step[self.ENTROPY]
            if mode == self.RESEED:
                drbg_op = drbg_test.DRBG_RESEED
                drbg_params = (entropy, input1, "")
            elif mode == self.GENERATE:
                drbg_op = drbg_test.DRBG_GENERATE
                generate_calls += 1
                if entropy:
                    raise ValueError('Got entropy during generate %r' % step)
                # The vectors only verify the second generate command. Only pass in
                # the result if it will match.
                check_result = generate_calls == 2
                expected_response = response if check_result else ''
                drbg_params = (input1, expected_response, check_result)
            else:
                raise ValueError("Invalid mode %r" % mode)
            self._add_test_input((drbg_op, drbg_params))

    def get_test_inputs(self):
        """Convert the lab input to the format required by drbg_test.

        Returns:
            a list of tuples (drbg_op, tuple of drbg_params)
        """
        for i, request_group in enumerate(self._request_vector[1][self.GROUPS]):
            response_bytes = request_group[self.RESPONSE_BITS] >> 3
            # The test expects each group to specify the response size in bytes.
            self._add_test_input((drbg_test.DRBG_GROUP_INIT, response_bytes))
            for j, test in enumerate(request_group[self.TEST_CASES]):
                response = self._get_expected_response(i, j)
                self._process_test_case(test, response)
        return self._test_inputs


    def _algo_get_formatted_results(self, results):
        """Format the results into the list the lab expects.

        Args:
            results: a list of tuples with the generated responses (tgid, tcid,
                     result_str)

        Returns:
            a list of dictionaries. There's a dictionary for each test group.
            Those contain the group id and a list of tests with the test case
            id and result string
        """
        formatted_results = []
        last_group = -1
        for result in results:
            group_id, test_id, response = result
            if group_id != last_group:
                last_group = group_id
                new_group = {}
                new_group[self.GROUP_ID] = group_id
                new_group[self.TEST_CASES] = []
                formatted_results.append(new_group)
            test_dict = {}
            test_dict[self.TEST_CASE_ID] = test_id
            test_dict[self.RESPONSE_KEY] = response
            # The group id counts from 1. Offset it to get the list index.
            formatted_results[group_id - 1][self.TEST_CASES].append(test_dict)
        return formatted_results
