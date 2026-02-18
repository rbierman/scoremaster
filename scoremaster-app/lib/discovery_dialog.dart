import 'dart:async';
import 'package:bonsoir/bonsoir.dart';
import 'package:flutter/material.dart';

class ScoreboardDiscoveryDialog extends StatefulWidget {
  final BonsoirDiscovery discovery;
  final Stream<BonsoirDiscoveryEvent> eventStream;
  final List<BonsoirService> discoveredServices;
  final Function(BonsoirService) onServiceSelected;

  const ScoreboardDiscoveryDialog({
    super.key,
    required this.discovery,
    required this.eventStream,
    required this.discoveredServices,
    required this.onServiceSelected,
  });

  @override
  State<ScoreboardDiscoveryDialog> createState() => _ScoreboardDiscoveryDialogState();
}

class _ScoreboardDiscoveryDialogState extends State<ScoreboardDiscoveryDialog> {
  late List<BonsoirService> _services;
  StreamSubscription<BonsoirDiscoveryEvent>? _subscription;

  @override
  void initState() {
    super.initState();
    _services = List.from(widget.discoveredServices);
    _subscription = widget.eventStream.listen((event) {
      if (event.service == null) return;
      final service = event.service!;
      debugPrint('[Discovery] Event: ${event.runtimeType} for ${service.name}');
      debugPrint('[Discovery] Service Data: ${service.toJson()}');

      setState(() {
        switch (event) {
          case BonsoirDiscoveryServiceFoundEvent():
            if (!_services.any((s) => s.name == service.name)) {
              _services.add(service);
              service.resolve(widget.discovery.serviceResolver);
            }
            break;
          case BonsoirDiscoveryServiceResolvedEvent():
            int index = _services.indexWhere((s) => s.name == service.name);
            if (index != -1) {
              _services[index] = service;
            }
            break;
          case BonsoirDiscoveryServiceLostEvent():
            _services.removeWhere((s) => s.name == service.name);
            break;
          default:
            break;
        }
      });
    });
  }

  @override
  void dispose() {
    _subscription?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: const Text('Select Scoreboard'),
      content: SizedBox(
        width: double.maxFinite,
        child: _services.isEmpty
            ? const Padding(
                padding: EdgeInsets.all(16.0),
                child: Text('No scoreboards found yet...'),
              )
            : ListView.builder(
                shrinkWrap: true,
                itemCount: _services.length,
                                  itemBuilder: (context, index) {
                                    final service = _services[index];
                                    final json = service.toJson();
                                    String subtitle = 'Resolving...';
                                    if (json['service.host'] != null && json['service.host'] != '0.0.0.0') {
                                      subtitle = '${json['service.host']}:${json['service.port']}';
                                    }

                                    String? version = service.attributes['version'];

                                    return ListTile(
                                      title: Row(
                                        children: [
                                          Expanded(child: Text(service.name)),
                                          if (version != null)
                                            Text(
                                              'v$version',
                                              style: Theme.of(context).textTheme.bodySmall?.copyWith(
                                                color: Colors.grey,
                                              ),
                                            ),
                                        ],
                                      ),
                                      subtitle: Text(subtitle),
                                      onTap: () => widget.onServiceSelected(service),
                                    );
                                  },              ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: const Text('Cancel'),
        ),
      ],
    );
  }
}
