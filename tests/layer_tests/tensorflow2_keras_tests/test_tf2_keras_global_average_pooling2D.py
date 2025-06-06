# Copyright (C) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import pytest
import tensorflow as tf

from common.tf2_layer_test_class import CommonTF2LayerTest


class TestKerasGlobalAvgPool2D(CommonTF2LayerTest):
    def create_keras_global_avg_pooling2D_net(self, input_names, input_shapes, input_type,
                                              dataformat, ir_version):
        """
                create TensorFlow 2 model with Keras GlobalAveragePooling2D operation
        """
        tf.keras.backend.clear_session()  # For easy reset of notebook state
        x = tf.keras.Input(shape=input_shapes[0][1:], name=input_names[0])
        y = tf.keras.layers.GlobalAveragePooling2D(data_format=dataformat)(x)
        tf2_net = tf.keras.Model(inputs=[x], outputs=[y])

        # TODO: add reference IR net. Now it is omitted since inference is more
        #  important and needs to be checked in the first
        ref_net = None
        return tf2_net, ref_net

    test_data_float32 = [
        dict(input_names=["x"], input_shapes=[[5, 9, 4, 2]], input_type=tf.float32,
             dataformat='channels_last'),
        dict(input_names=["x"], input_shapes=[[5, 3, 8, 8]], input_type=tf.float32,
             dataformat='channels_first'),
    ]

    @pytest.mark.parametrize("params", test_data_float32)
    @pytest.mark.nightly
    @pytest.mark.precommit
    def test_keras_global_avg_pooling2D_float32(self, params, ie_device, precision, temp_dir,
                                                ir_version):
        self._test(*self.create_keras_global_avg_pooling2D_net(**params, ir_version=ir_version),
                   ie_device, precision, temp_dir=temp_dir, ir_version=ir_version,
                   **params)
