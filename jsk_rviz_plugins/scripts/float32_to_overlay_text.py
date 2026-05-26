#!/usr/bin/env python3

import rclpy

from rclpy.parameter import Parameter
from std_msgs.msg import Float32
from threading import Lock
from jsk_rviz_plugins.overlay_text_interface import OverlayTextInterface

g_lock = Lock()
g_msg = None

def callback(msg):
    global g_msg, g_lock
    with g_lock:
        g_msg = msg

def config_callback(config, level):
    global g_format
    g_format = config.format
    return config

def publish_text():
    global g_lock, g_msg, g_format
    with g_lock:
        if not g_msg:
            return
        text_interface.publish(g_format.format(g_msg.data))

def publish_text_multi():
    global g_lock, multi_topic_msgs, g_format
    with g_lock:
        if all([msg for topic, msg in multi_topic_msgs.items()]):
            text_interface.publish(g_format.format(sum([msg.data for topic, msg in multi_topic_msgs.items()])))
        
multi_topic_msgs = dict()
        
class MultiTopicCallback():
    def __init__(self, topic):
        self.topic = topic
        global multi_topic_msgs
        multi_topic_msgs[self.topic] = None
    def callback(self, msg):
        global multi_topic_msgs
        with g_lock:
            multi_topic_msgs[self.topic] = msg
        
if __name__ == "__main__":
    rclpy.init()
    node = rclpy.create_node("float32_to_overlay_text")
    text_interface = OverlayTextInterface(node, "~text")
    node.declare_parameter("multi_topics", Parameter.Type.STRING_ARRAY)
    node.declare_parameter("format", "value: {0}")
    multi_topics = node.get_parameter("multi_topics").value
    g_format = node.get_parameter("format").value
    try:
        if multi_topics:
            subs = []
            for topic in multi_topics:
                multi_callback = MultiTopicCallback(topic)
                subs.append(node.create_subscription(Float32, topic, multi_callback.callback, 10))
            node.create_timer(0.1, publish_text_multi)
        else:
            sub = node.create_subscription(Float32, "~/input", callback, 10)
            node.create_timer(0.1, publish_text)
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()
