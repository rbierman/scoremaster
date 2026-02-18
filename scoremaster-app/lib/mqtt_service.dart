import 'dart:convert';
import 'dart:async';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'scoreboard_state.dart';

class MqttService {
  final MqttServerClient client;
  final _stateController = StreamController<ScoreboardState>.broadcast();

  Stream<ScoreboardState> get stateStream => _stateController.stream;

  MqttService(String host, String clientId)
      : client = MqttServerClient(host, clientId) {
    client.logging(on: false);
    client.keepAlivePeriod = 20;
    client.onDisconnected = _onDisconnected;
    client.onConnected = _onConnected;
    client.onSubscribed = _onSubscribed;
  }

  Future<bool> connect() async {
    try {
      await client.connect();
      return true;
    } catch (e) {
      print('MQTT connection failed: $e');
      return false;
    }
  }

  void _onConnected() {
    print('Connected to MQTT broker');
    client.subscribe('hockey/scoreboard/state', MqttQos.atMostOnce);
    client.updates!.listen((List<MqttReceivedMessage<MqttMessage>> c) {
      final MqttPublishMessage recMess = c[0].payload as MqttPublishMessage;
      final pt = MqttPublishPayload.bytesToStringAsString(recMess.payload.message);

      try {
        final Map<String, dynamic> data = jsonDecode(pt);
        final state = ScoreboardState.fromJson(data);
        _stateController.add(state);
      } catch (e) {
        print('Error parsing scoreboard state: $e');
      }
    });
  }

  void _onDisconnected() {
    print('Disconnected from MQTT broker');
  }

  void _onSubscribed(String topic) {
    print('Subscribed to topic: $topic');
  }

  void sendCommand(String command, {dynamic value, int? delta}) {
    final Map<String, dynamic> payload = {'command': command};
    if (value != null) payload['value'] = value;
    if (delta != null) payload['delta'] = delta;

    final builder = MqttClientPayloadBuilder();
    builder.addString(jsonEncode(payload));
    client.publishMessage('hockey/scoreboard/command', MqttQos.atMostOnce, builder.payload!);
  }

  void dispose() {
    client.disconnect();
    _stateController.close();
  }
}
